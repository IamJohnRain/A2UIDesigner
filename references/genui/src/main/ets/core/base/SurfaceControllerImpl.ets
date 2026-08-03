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

import { A2UIMessageType } from './A2UIMessage';
import { BackGestureEnabledListener, BackGestureObservableController } from './BackGestureTypes';
import { CatalogImpl } from './CatalogImpl';
import { RenderIdGenerator } from './RenderIdGenerator';
import { CustomComponentFactory } from '../components/A2UI/CustomComponentFactory';
import { CrossLanguageAttributeBridge } from '../functions/CrossLanguageAttributeBridge';
import { FunctionBridge, LocalFunctionErrorEvent, LocalFunctionErrorListener } from '../functions/FunctionBridge';
import { SchemaErrorInfo, SchemaErrorInfoManager, SchemaItemType } from '../schema/SchemaErrorInfoManager';
import { A2UIValueType } from '../types/types';
import {
  ClientToServerActionMessage,
  CrossLanguageAttributeRequest,
  NativeActionEventRequest,
  NativeEngineBridge,
  NativeProcessOptions,
  ThemeMode,
  Breakpoint,
  NativeSchemaWarningRequest,
  NativeRuntimeErrorRequest
} from './NativeEngineBridge';
import { BreakpointUtils } from '../common/BreakpointUtils';
import { SurfaceErrorCode, SurfaceEventType } from './SurfaceTypes';

import { NodeContent, UIContext } from '@kit.ArkUI';
import { deviceInfo } from '@kit.BasicServicesKit';
import {
  SchemaWarningInfo,
  SurfaceController,
  SurfaceControllerOption
} from '../../interface/SurfaceController';
import { A2UI_EXTENDED_CATALOG_ID, CapabilitiesCore } from './CapabilitiesCore';
import { Catalog } from '../../interface/Catalog';

export { SurfaceErrorCode, SurfaceEventType } from './SurfaceTypes';
export { ThemeMode, Breakpoint } from './NativeEngineBridge';
export { SchemaWarningInfo } from '../../interface/SurfaceController';

export declare type ActionReceiver = (action: string) => void;
export declare type ErrorCallback = (code: SurfaceErrorCode, errorMsg: string) => void;
export declare type SurfaceEventCallback = (
  eventType: SurfaceEventType,
  controller: SurfaceControllerImpl,
  surfaceId: string
) => void;

const EMPTY_SURFACE_EVENT_CALLBACK: SurfaceEventCallback = (
  _eventType: SurfaceEventType,
  _controller: SurfaceControllerImpl,
  _surfaceId?: string
) => {
};

interface SurfaceSchemaWarningPayload {
    type: 'schemaWarning';
    warningCount: number;
    warnings: SurfaceSchemaWarningItem[];
}

interface SurfaceSchemaWarningItem {
    code: string;
    message: string;
    path?: string;
    itemType?: SchemaItemType;
    itemName?: string;
}

interface SurfaceErrorMessageContext {
    componentType?: string;
    componentId?: string;
}

export class SurfaceControllerImpl implements SurfaceController, BackGestureObservableController {
    private static readonly MIN_SUPPORTED_API_VERSION: number = 20;
    private static actionBridgeInstalled: boolean = false;
    private static schemaWarningBridgeInstalled: boolean = false;
    private static crossLanguageAttributeInstalled: boolean = false;
    private static runtimeErrorBridgeInstalled: boolean = false;
    private static controllerByRenderId: Map<number, SurfaceControllerImpl> = new Map();

    private catalog: CatalogImpl;
    private readonly isExtend: boolean;
    private checkSchema: boolean;
    private readonly uiContext: UIContext;
    // used to notify the caller about the user action to send
    private onAction?: ActionReceiver;
    // used to notify the caller about the handling error
    private onError?: ErrorCallback;
    // used to notify the caller about the message handling result
    private onSurfaceEvent: SurfaceEventCallback;

    private currentRootSlot?: NodeContent;

    private isFollowSystemColorMode: boolean = true;

    // Unique render ID for this SurfaceControllerImpl instance
    private readonly renderId: number;
    private readonly apiVersion: number;
    private readonly apiSupported: boolean;
    private destroyed: boolean = false;
    private lastSurfaceCode: SurfaceErrorCode = SurfaceErrorCode.NO_ERROR;
    private lastSurfaceMessage: string = '';
    private pendingNativeSchemaWarnings: SchemaErrorInfo[] = [];
    private nativeSchemaWarningFlushScheduled: boolean = false;
    private pendingReportedSchemaWarnings: SchemaWarningInfo[] = [];
    private reportedSchemaWarningFlushScheduled: boolean = false;

    // Theme state tracking
    private currentThemeMode: ThemeMode = ThemeMode.LIGHT;
    private currentBreakpoint: Breakpoint = Breakpoint.SM;
    private localFunctionErrorListener: LocalFunctionErrorListener = (event: LocalFunctionErrorEvent): void => {
      this.handleLocalFunctionError(event);
    };

    constructor(option: SurfaceControllerOption) {
        this.checkSchema = true;
        this.catalog = CatalogImpl.fromPublicOrEmpty(option.catalog);
        this.isExtend = SurfaceControllerImpl.isExtendedCatalog(this.catalog);
        this.onSurfaceEvent = option.eventCallback ?? EMPTY_SURFACE_EVENT_CALLBACK;
        this.uiContext = option.uiContext;

        // Generate unique render ID
        this.renderId = RenderIdGenerator.getInstance().generateId();
        this.apiVersion = deviceInfo.sdkApiVersion;
        this.apiSupported = this.apiVersion >= SurfaceControllerImpl.MIN_SUPPORTED_API_VERSION;
        console.log(
          `SurfaceController: Constructor - Generated renderId=${this.renderId}, apiVersion=${this.apiVersion}`
        );
        SchemaErrorInfoManager.getInstance().registerWarningGate(this.renderId, this.checkSchema);
        if (!this.apiSupported) {
            console.warn(
              `[SurfaceController] native renderer disabled: apiVersion=${this.apiVersion}, ` +
              `minSupported=${SurfaceControllerImpl.MIN_SUPPORTED_API_VERSION}, renderId=${this.renderId}`
            );
        } else {
            this.syncCustomComponentUIContext();
            // Initialize RenderSlot in C++ layer
            console.log(`SurfaceController: initRenderSlot - renderId=${this.renderId}`);
            NativeEngineBridge.initRenderSlot(this.renderId);
            NativeEngineBridge.setApiVersion(this.renderId, this.apiVersion);
            SurfaceControllerImpl.installActionBridgeIfNeeded();
            SurfaceControllerImpl.installSchemaWarningBridgeIfNeeded();
            SurfaceControllerImpl.installCrossLanguageAttributeIfNeeded();
            SurfaceControllerImpl.installRuntimeErrorBridgeIfNeeded();
            SurfaceControllerImpl.controllerByRenderId.set(this.renderId, this);
            FunctionBridge.install();
            FunctionBridge.registerLocalFunctionErrorListener(this.localFunctionErrorListener);
        }
    }

    private syncCustomComponentUIContext(): void {
        CustomComponentFactory.InstallCustomComponent(this.uiContext);
    }

    private static isExtendedCatalog(catalog: CatalogImpl): boolean {
      return catalog.getCatalogId().toLowerCase() === A2UI_EXTENDED_CATALOG_ID.toLowerCase();
    }

    private static installActionBridgeIfNeeded(): void {
        if (SurfaceControllerImpl.actionBridgeInstalled) {
            return;
        }
        NativeEngineBridge.installActionBridge(SurfaceControllerImpl.dispatchNativeAction);
        SurfaceControllerImpl.actionBridgeInstalled = true;
    }

    private static installSchemaWarningBridgeIfNeeded(): void {
        if (SurfaceControllerImpl.schemaWarningBridgeInstalled) {
            return;
        }
        NativeEngineBridge.installSchemaWarningBridge(SurfaceControllerImpl.dispatchNativeSchemaWarning);
        SurfaceControllerImpl.schemaWarningBridgeInstalled = true;
    }

    private static installCrossLanguageAttributeIfNeeded(): void {
        if (SurfaceControllerImpl.crossLanguageAttributeInstalled) {
            return;
        }
        SurfaceControllerImpl.crossLanguageAttributeInstalled = NativeEngineBridge.installCrossLanguageAttributeCallback(
            SurfaceControllerImpl.handleCrossLanguageAttribute
        );
        console.info(
            `SurfaceController: installCrossLanguageAttribute ` +
            `result=${SurfaceControllerImpl.crossLanguageAttributeInstalled}`
        );
    }

    private static handleCrossLanguageAttribute(request: CrossLanguageAttributeRequest): void {
      CrossLanguageAttributeBridge.applyAttribute(
        request.renderId,
        request.componentId,
        request.nodeUniqueId,
        request.componentType,
        request.attributeName,
        request.floatValue,
        request.stringValue,
        request.payloadJson,
        request.reset
      );
    }

    private static installRuntimeErrorBridgeIfNeeded(): void {
        if (SurfaceControllerImpl.runtimeErrorBridgeInstalled) {
            return;
        }
        NativeEngineBridge.installRuntimeErrorBridge(SurfaceControllerImpl.dispatchNativeRuntimeError);
        SurfaceControllerImpl.runtimeErrorBridgeInstalled = true;
    }

    public static getControllerByRenderId(renderId: number): SurfaceControllerImpl | undefined {
      return SurfaceControllerImpl.controllerByRenderId.get(renderId);
    }

    public static fromPublicOrNull(controller: SurfaceController | null | undefined): SurfaceControllerImpl | null {
      if (controller === null || controller === undefined) {
        return null;
      }
      if (controller instanceof SurfaceControllerImpl) {
        return controller;
      }
      console.error('[SurfaceControllerImpl] SurfaceController is not an instance of SurfaceControllerImpl, returning null.');
      return null;
    }

    protected supportsMultipleSurfaces(): boolean {
      return false;
    }

    protected getMaxSurfaceCount(): number | null {
      return null;
    }

    protected getCurrentSurfaceIds(): string[] {
      if (this.destroyed) {
        return [];
      }
      if (!this.apiSupported) {
        return [];
      }
      let nativeIds = NativeEngineBridge.getSurfaceIds(this.renderId);
      return nativeIds !== undefined ? nativeIds : [];
    }

    protected getTopSurfaceId(): string {
      if (this.destroyed) {
        return '';
      }
      if (!this.apiSupported) {
        return '';
      }
      let nativeId = NativeEngineBridge.getLatestSurfaceId(this.renderId);
      return nativeId !== undefined ? nativeId : '';
    }

    protected recordSurfaceResult(code: SurfaceErrorCode, message?: string): SurfaceErrorCode {
      this.lastSurfaceCode = code;
      this.lastSurfaceMessage = message ?? '';
      return code;
    }

    protected getRenderIdForLog(): number {
      return this.renderId;
    }

    protected isDestroyed(): boolean {
      return this.destroyed;
    }

    /**
     * Binds/unbinds schema-warning routing for a surface on CREATE/DELETE messages.
     * Multi-surface subclasses override to also maintain their tracked surface stack.
     */
    protected applySurfaceLifecycle(surfaceId: string, messageType: A2UIMessageType): void {
      switch (messageType) {
        case A2UIMessageType.CREATE_SURFACE:
          SchemaErrorInfoManager.getInstance().bindSurface(this.renderId, surfaceId);
          SchemaErrorInfoManager.getInstance().registerWarningEmitter(surfaceId, (warnings: SchemaErrorInfo[]) => {
            this.emitSchemaWarnings(warnings);
          });
          break;
        case A2UIMessageType.DELETE_SURFACE:
          SchemaErrorInfoManager.getInstance().unbindSurface(this.renderId, surfaceId);
          SchemaErrorInfoManager.getInstance().unregisterWarningEmitter(surfaceId);
          break;
        default:
          break;
      }
    }

    protected describeSurfaceStackForLog(): string {
      let nativeIds = NativeEngineBridge.getSurfaceIds(this.renderId);
      if (nativeIds === undefined) {
        return '[]';
      }
      return nativeIds.length === 0 ? '[]' : `[${nativeIds.join(', ')}]`;
    }

    protected getCleanupSurfaceIds(): string[] {
      let nativeIds = NativeEngineBridge.getSurfaceIds(this.renderId);
      return nativeIds !== undefined ? nativeIds : [];
    }

    private describeProcessOptionsForLog(): string {
      let maxSurfaceCount = this.getMaxSurfaceCount();
      return `supportsMultipleSurfaces=${this.supportsMultipleSurfaces()}, maxSurfaceCount=${maxSurfaceCount ?? 'null'}, ` +
        `isExtend=${this.isExtend}, checkSchema=${this.checkSchema}`;
    }

    private createDslPreviewForLog(dsl: string): string {
      const normalized = dsl.replace(/\s+/g, ' ').trim();
      if (normalized.length <= 120) {
        return normalized;
      }
      return `${normalized.substring(0, 117)}...`;
    }

    protected static describeMessageTypeForLog(messageType: A2UIMessageType): string {
      switch (messageType) {
        case A2UIMessageType.CREATE_SURFACE:
          return 'CREATE_SURFACE';
        case A2UIMessageType.UPDATE_COMPONENTS:
          return 'UPDATE_COMPONENTS';
        case A2UIMessageType.UPDATE_DATA_MODEL:
          return 'UPDATE_DATA_MODEL';
        case A2UIMessageType.DELETE_SURFACE:
          return 'DELETE_SURFACE';
        default:
          return `UNKNOWN(${messageType})`;
      }
    }

    protected logSurfaceLifecycle(event: string, extra?: string): void {
      let details = `[SurfaceController] ${event}, renderId=${this.renderId}, destroyed=${this.destroyed}, ` +
        `apiVersion=${this.apiVersion}, apiSupported=${this.apiSupported}, ` +
        `bound=${this.currentRootSlot !== undefined}, surfaceIds=${this.describeSurfaceStackForLog()}`;
      if (extra !== undefined && extra.length > 0) {
        details += `, ${extra}`;
      }
      console.info(details);
    }

    protected createUnsupportedApiMessage(): string {
      return `A2UI native renderer requires API ${SurfaceControllerImpl.MIN_SUPPORTED_API_VERSION} or later, ` +
        `current API is ${this.apiVersion}`;
    }

    private static dispatchNativeAction(request: NativeActionEventRequest): void {
        if (request === undefined || request === null || typeof request.renderId !== 'number') {
            return;
        }

        const controller = SurfaceControllerImpl.controllerByRenderId.get(request.renderId);
        if (controller === undefined) {
            return;
        }
        controller.emitActionEvent(request);
    }

    private static dispatchNativeSchemaWarning(request: NativeSchemaWarningRequest): void {
        if (request === undefined || request === null || typeof request.renderId !== 'number') {
            return;
        }

        const controller = SurfaceControllerImpl.controllerByRenderId.get(request.renderId);
        if (controller === undefined || controller.destroyed) {
            return;
        }

        const schemaErrorInfoManager = SchemaErrorInfoManager.getInstance();
        const warningCountBefore = schemaErrorInfoManager.getWarningCount();
        const itemType: SchemaItemType | undefined = SurfaceControllerImpl.normalizeSchemaItemType(request.itemType);
        schemaErrorInfoManager.withSchemaWarningContext(
          request.renderId,
          request.surfaceId,
          () => {
            schemaErrorInfoManager.recordSchemaWarning(
              request.code,
              request.message,
              request.path,
              itemType,
              request.itemName
            );
          }
        );
        if (!schemaErrorInfoManager.isCollectingWarningsForMessage()) {
          const newWarnings: SchemaErrorInfo[] = schemaErrorInfoManager.getWarnings().slice(warningCountBefore);
          controller.enqueueNativeSchemaWarnings(newWarnings);
        }
    }

    private static dispatchNativeRuntimeError(request: NativeRuntimeErrorRequest): void {
        if (request === undefined || request === null || typeof request.renderId !== 'number') {
            return;
        }

        const controller = SurfaceControllerImpl.controllerByRenderId.get(request.renderId);
        if (controller === undefined || controller.destroyed) {
            return;
        }
        controller.handleNativeRuntimeError(request);
    }

    private static normalizeSchemaItemType(itemType: SchemaItemType | undefined): SchemaItemType | undefined {
      if (itemType === 'message' || itemType === 'component' || itemType === 'function' || itemType === 'schema') {
        return itemType;
      }
      return undefined;
    }

    private static normalizeMessageContextValue(value?: string): string | undefined {
      const normalized: string | undefined = value?.trim();
      if (normalized !== undefined && normalized.length > 0) {
        return normalized;
      }
      return undefined;
    }

    private static appendComponentContextToMessage(
      message: string,
      context: SurfaceErrorMessageContext
    ): string {
      const details: string[] = [];
      const componentType: string | undefined = SurfaceControllerImpl.normalizeMessageContextValue(context.componentType);
      const componentId: string | undefined = SurfaceControllerImpl.normalizeMessageContextValue(context.componentId);
      if (componentType !== undefined) {
        details.push(`componentType=${componentType}`);
      }
      if (componentId !== undefined) {
        details.push(`componentId=${componentId}`);
      }
      const missingDetails: string[] = details.filter((detail: string) => message.indexOf(detail) < 0);
      if (missingDetails.length === 0) {
        return message;
      }
      return `${message} (${missingDetails.join(', ')})`;
    }

    private clearSchemaWarningsForMessage(): void {
      SchemaErrorInfoManager.getInstance().clear();
    }

    private createSchemaWarningPayloadItem(warning: SchemaErrorInfo): SurfaceSchemaWarningItem {
      const payloadItem: SurfaceSchemaWarningItem = {
        code: warning.code,
        message: warning.message
      };
      if (warning.path !== undefined && warning.path.length > 0) {
        payloadItem.path = warning.path;
      }
      if (warning.itemType !== undefined) {
        payloadItem.itemType = warning.itemType;
      }
      if (warning.itemName !== undefined && warning.itemName.length > 0) {
        payloadItem.itemName = warning.itemName;
      }
      return payloadItem;
    }

    private buildSchemaWarningPayloadFromWarnings(warnings: SchemaErrorInfo[]): string {
      const payload: SurfaceSchemaWarningPayload = {
        type: 'schemaWarning',
        warningCount: warnings.length,
        warnings: []
      };
      warnings.forEach((warning: SchemaErrorInfo) => {
        payload.warnings.push(this.createSchemaWarningPayloadItem(warning));
      });
      return JSON.stringify(payload);
    }

    private emitSchemaWarnings(warnings: SchemaErrorInfo[]): void {
      if (!this.checkSchema || this.destroyed || this.onError === undefined || warnings.length === 0) {
        return;
      }
      this.logSurfaceLifecycle('schema warnings reported', `warningCount=${warnings.length}`);
      this.onError(SurfaceErrorCode.SCHEMA_WARNING, this.buildSchemaWarningPayloadFromWarnings(warnings));
    }

    private emitSchemaWarningsIfNeeded(): void {
      if (!SchemaErrorInfoManager.getInstance().hasWarnings()) {
        return;
      }
      this.emitSchemaWarnings(SchemaErrorInfoManager.getInstance().getWarnings());
    }

    private enqueueNativeSchemaWarnings(warnings: SchemaErrorInfo[]): void {
      if (!this.checkSchema || this.destroyed || this.onError === undefined || warnings.length === 0) {
        return;
      }
      warnings.forEach((warning: SchemaErrorInfo) => {
        this.pendingNativeSchemaWarnings.push(warning);
      });
      if (this.nativeSchemaWarningFlushScheduled) {
        return;
      }
      this.nativeSchemaWarningFlushScheduled = true;
      const renderId: number = this.renderId;
      setTimeout((): void => {
        const controller: SurfaceControllerImpl | undefined = SurfaceControllerImpl.controllerByRenderId.get(renderId);
        if (controller !== undefined) {
          controller.flushPendingNativeSchemaWarnings();
        }
      }, 0);
    }

    private flushPendingNativeSchemaWarnings(): void {
      this.nativeSchemaWarningFlushScheduled = false;
      if (this.pendingNativeSchemaWarnings.length === 0) {
        return;
      }
      const warnings: SchemaErrorInfo[] = this.pendingNativeSchemaWarnings;
      this.pendingNativeSchemaWarnings = [];
      this.emitSchemaWarnings(warnings);
    }

    private enqueueReportedSchemaWarning(schemaWarningInfo: SchemaWarningInfo): void {
      this.pendingReportedSchemaWarnings.push({
        code: schemaWarningInfo.code,
        errorMsg: schemaWarningInfo.errorMsg
      });
      if (this.reportedSchemaWarningFlushScheduled) {
        return;
      }
      this.reportedSchemaWarningFlushScheduled = true;
      const renderId: number = this.renderId;
      setTimeout((): void => {
        const controller: SurfaceControllerImpl | undefined = SurfaceControllerImpl.controllerByRenderId.get(renderId);
        if (controller !== undefined) {
          controller.flushPendingReportedSchemaWarnings();
        }
      }, 0);
    }

    private flushPendingReportedSchemaWarnings(): void {
      this.reportedSchemaWarningFlushScheduled = false;
      if (this.pendingReportedSchemaWarnings.length === 0) {
        return;
      }
      const warnings: SchemaWarningInfo[] = this.pendingReportedSchemaWarnings;
      this.pendingReportedSchemaWarnings = [];
      if (!this.checkSchema || this.destroyed || this.onError === undefined) {
        return;
      }
      warnings.forEach((warning: SchemaWarningInfo) => {
        this.onError?.(warning.code, warning.errorMsg);
      });
    }

    private emitActionEvent(request: NativeActionEventRequest): void {
        if (this.destroyed) {
            return;
        }
        if (this.onAction === undefined) {
            this.logSurfaceLifecycle(
              'emitActionEvent failed',
              `reason=onAction callback not registered, surfaceId=${request.surfaceId}, action=${request.name}`
            );
            this.reportError(
              SurfaceErrorCode.ACTION_NOT_REGISTER,
              'onAction callback is not registered',
              { componentId: request.sourceComponentId }
            );
            return;
        }

        let context: A2UIValueType = {} as Record<string, Object>;
        if (request.context !== undefined) {
            context = request.context;
        }

        const timestamp =
            request.timestamp !== undefined && request.timestamp.length > 0 ? request.timestamp : new Date().toISOString();
        const actionPayload: ClientToServerActionMessage = {
            version: CapabilitiesCore.getDefaultA2UIProtocolVersion(),
            action: {
                name: request.name,
                surfaceId: request.surfaceId,
                sourceComponentId: request.sourceComponentId,
                timestamp: timestamp,
                context: context
            }
        };
        this.onAction(JSON.stringify(actionPayload));
    }

    private reportError(
      errorType: SurfaceErrorCode,
      runtimeErrorMsg: string,
      context: SurfaceErrorMessageContext = {}
    ): void {
      const errorMessage: string = SurfaceControllerImpl.appendComponentContextToMessage(runtimeErrorMsg, context);
      console.error(
        `[SurfaceController] onError reported, renderId=${this.renderId}, ` +
        `errorCode=${errorType}, errorMsg=${errorMessage}`
      );
      this.onError?.(errorType, errorMessage);
    }

    private handleLocalFunctionError(event: LocalFunctionErrorEvent): void {
      if (this.destroyed) {
        console.warn(
          `[SurfaceController] local function error ignored: controller destroyed, renderId=${this.renderId}, ` +
          `functionName=${event.request.functionName}`
        );
        return;
      }
      const request = event.request;
      if (request.surfaceId.length === 0) {
        console.warn(
          `[SurfaceController] local function error ignored: empty surfaceId, renderId=${this.renderId}, ` +
          `componentId=${request.componentId}, functionName=${request.functionName}`
        );
        return;
      }
      const surfaceIds = this.getCurrentSurfaceIds();
      if (!surfaceIds.includes(request.surfaceId)) {
        console.warn(
          `[SurfaceController] local function error ignored: surface not owned by controller, renderId=${this.renderId}, ` +
          `surfaceId=${request.surfaceId}, functionName=${request.functionName}`
        );
        return;
      }
      const runtimeErrorCode: number = event.errorCode;
      const runtimeErrorMessage = event.errorMessage.length > 0 ? event.errorMessage : 'local function invoke failed';
      this.logSurfaceLifecycle(
        'local function failed',
        `surfaceId=${request.surfaceId}, componentId=${request.componentId}, functionName=${request.functionName}, ` +
        `errorCode=${runtimeErrorCode}, errorMessage=${runtimeErrorMessage}`
      );
      this.reportError(runtimeErrorCode as SurfaceErrorCode, runtimeErrorMessage, {
        componentId: request.componentId
      });
    }

    private handleNativeRuntimeError(request: NativeRuntimeErrorRequest): void {
      const runtimeErrorCode = request.errorCode > 0 ? request.errorCode : SurfaceErrorCode.NATIVE_PROCESS_FAILED;
      const runtimeErrorMessage =
        request.errorMessage.length > 0 ? request.errorMessage : 'native runtime error';
      this.logSurfaceLifecycle(
        'native runtime error',
        `surfaceId=${request.surfaceId}, componentId=${request.componentId}, ` +
        `errorCode=${runtimeErrorCode}, source=${request.source ?? 'unknown'}, errorMessage=${runtimeErrorMessage}`
      );
      this.reportError(runtimeErrorCode as SurfaceErrorCode, runtimeErrorMessage, {
        componentId: request.componentId
      });
    }

    private reportSurfacePolicyFailure(code: SurfaceErrorCode, message: string): void {
      this.logSurfaceLifecycle('policy rejected message', `code=${code}, message=${message}`);
      this.onError?.(SurfaceErrorCode.NO_SURFACE_MATCHED, message || 'surface policy rejected message');
    }

    private createNativeProcessOptions(): NativeProcessOptions {
      return {
        supportsMultipleSurfaces: this.supportsMultipleSurfaces(),
        maxSurfaceCount: this.getMaxSurfaceCount() ?? -1,
        isExtend: this.isExtend
      };
    }

    private isSurfacePolicyCode(code: number | undefined): boolean {
      return code === SurfaceErrorCode.MULTI_SURFACE_DISABLED ||
        code === SurfaceErrorCode.MULTI_SURFACE_MAX_SURFACE_LIMIT_REACHED ||
        code === SurfaceErrorCode.MULTI_SURFACE_ALREADY_EXISTS;
    }

    public handleMessage(dsl: string): void {
        if (this.destroyed) {
            this.logSurfaceLifecycle('handleMessage ignored', 'reason=controller destroyed');
            return;
        }
        if (!this.apiSupported) {
            const message = this.createUnsupportedApiMessage();
            this.lastSurfaceCode = this.recordSurfaceResult(SurfaceErrorCode.NATIVE_PROCESS_FAILED, message);
            this.logSurfaceLifecycle('handleMessage ignored', `reason=unsupported api version, ${message}`);
            this.onError?.(SurfaceErrorCode.NATIVE_PROCESS_FAILED, message);
            return;
        }

        SchemaErrorInfoManager.getInstance().withMessageWarningCollection(() => {
          this.handleReceiveMessage(dsl);
        });
    }

    private handleReceiveMessage(dsl: string): void {
        this.clearSchemaWarningsForMessage();

        this.logSurfaceLifecycle(
          'handleMessage start',
          `dslLength=${dsl.length}, dslPreview=${this.createDslPreviewForLog(dsl)}, options=${this.describeProcessOptionsForLog()}`
        );
        this.syncCustomComponentUIContext();
        let processResult = NativeEngineBridge.processMessage(
          this.renderId,
          dsl,
          this.catalog,
          this.createNativeProcessOptions()
        );
        if (processResult === undefined || processResult === null) {
            this.lastSurfaceCode = this.recordSurfaceResult(SurfaceErrorCode.NATIVE_PROCESS_FAILED, 'native process failed');
            this.logSurfaceLifecycle('handleMessage failed', 'reason=native process result missing');
            this.emitSchemaWarningsIfNeeded();
            this.onError?.(SurfaceErrorCode.NATIVE_PROCESS_FAILED, 'native process failed');
            return;
        }

        if (processResult.success !== true) {
            let nativeErrorCode: string = '';
            let errorMessage: string = 'native process failed';
            if (processResult.errorCode !== undefined) {
                nativeErrorCode = processResult.errorCode;
            }
            if (processResult.errorMessage !== undefined && processResult.errorMessage.length > 0) {
                errorMessage = processResult.errorMessage;
            }
            if (nativeErrorCode === 'VERSION_INVALID') {
                this.lastSurfaceCode =
                    this.recordSurfaceResult(SurfaceErrorCode.SCHEMA_VERSION_INVALID, errorMessage);
                this.logSurfaceLifecycle(
                  'handleMessage failed',
                  `reason=version invalid, dslPreview=${this.createDslPreviewForLog(dsl)}`
                );
                this.onError?.(SurfaceErrorCode.SCHEMA_VERSION_INVALID, errorMessage);
                return;
            }
            if (nativeErrorCode === 'UNSUPPORTED_PROTOCOL_VERSION') {
                this.lastSurfaceCode = this.recordSurfaceResult(
                    SurfaceErrorCode.UNSUPPORTED_PROTOCOL_VERSION,
                    errorMessage
                );
                this.logSurfaceLifecycle(
                  'handleMessage failed',
                  `reason=unsupported protocol version, dslPreview=${this.createDslPreviewForLog(dsl)}`
                );
                this.onError?.(SurfaceErrorCode.UNSUPPORTED_PROTOCOL_VERSION, errorMessage);
                return;
            }
            if (nativeErrorCode === 'DSL_EMPTY') {
                this.lastSurfaceCode =
                    this.recordSurfaceResult(SurfaceErrorCode.SCHEMA_DSL_EMPTY, errorMessage);
                this.logSurfaceLifecycle(
                  'handleMessage failed',
                  `reason=dsl empty, dslPreview=${this.createDslPreviewForLog(dsl)}`
                );
                this.emitSchemaWarningsIfNeeded();
                this.onError?.(SurfaceErrorCode.SCHEMA_DSL_EMPTY, errorMessage);
                return;
            }
            if (nativeErrorCode === 'JSON_PARSE_FAILED') {
                this.lastSurfaceCode =
                    this.recordSurfaceResult(SurfaceErrorCode.SCHEMA_JSON_PARSE_FAILED, errorMessage);
                this.logSurfaceLifecycle(
                  'handleMessage failed',
                  `reason=json parse failed, dslPreview=${this.createDslPreviewForLog(dsl)}`
                );
                this.onError?.(SurfaceErrorCode.SCHEMA_JSON_PARSE_FAILED, errorMessage);
                return;
            }
            if (nativeErrorCode === 'ROOT_NOT_OBJECT') {
                this.lastSurfaceCode =
                    this.recordSurfaceResult(SurfaceErrorCode.SCHEMA_ROOT_NOT_OBJECT, errorMessage);
                this.logSurfaceLifecycle(
                  'handleMessage failed',
                  `reason=root not object, dslPreview=${this.createDslPreviewForLog(dsl)}`
                );
                this.onError?.(SurfaceErrorCode.SCHEMA_ROOT_NOT_OBJECT, errorMessage);
                return;
            }
            if (nativeErrorCode === 'MESSAGE_OPERATION_INVALID') {
                this.lastSurfaceCode =
                    this.recordSurfaceResult(SurfaceErrorCode.SCHEMA_MESSAGE_OPERATION_INVALID, errorMessage);
                this.logSurfaceLifecycle(
                  'handleMessage failed',
                  `reason=message operation invalid, dslPreview=${this.createDslPreviewForLog(dsl)}`
                );
                this.onError?.(SurfaceErrorCode.SCHEMA_MESSAGE_OPERATION_INVALID, errorMessage);
                return;
            }
            if (nativeErrorCode === 'MESSAGE_MULTIPLE_BODIES') {
                this.lastSurfaceCode =
                    this.recordSurfaceResult(SurfaceErrorCode.SCHEMA_MESSAGE_MULTIPLE_BODIES, errorMessage);
                this.logSurfaceLifecycle(
                  'handleMessage failed',
                  `reason=message multiple bodies, dslPreview=${this.createDslPreviewForLog(dsl)}`
                );
                this.onError?.(SurfaceErrorCode.SCHEMA_MESSAGE_MULTIPLE_BODIES, errorMessage);
                return;
            }
            if (nativeErrorCode === 'MESSAGE_BODY_INVALID') {
                this.lastSurfaceCode =
                    this.recordSurfaceResult(SurfaceErrorCode.SCHEMA_MESSAGE_BODY_INVALID, errorMessage);
                this.logSurfaceLifecycle(
                  'handleMessage failed',
                  `reason=message body invalid, dslPreview=${this.createDslPreviewForLog(dsl)}`
                );
                this.onError?.(SurfaceErrorCode.SCHEMA_MESSAGE_BODY_INVALID, errorMessage);
                return;
            }
            if (nativeErrorCode === 'SURFACE_ID_MISSING') {
                this.lastSurfaceCode =
                    this.recordSurfaceResult(SurfaceErrorCode.SCHEMA_SURFACE_ID_MISSING, errorMessage);
                this.logSurfaceLifecycle(
                  'handleMessage failed',
                  `reason=surfaceId missing, dslPreview=${this.createDslPreviewForLog(dsl)}`
                );
                this.onError?.(SurfaceErrorCode.SCHEMA_SURFACE_ID_MISSING, errorMessage);
                return;
            }
            if (nativeErrorCode === 'COMPONENTS_INVALID') {
                this.lastSurfaceCode =
                    this.recordSurfaceResult(SurfaceErrorCode.SCHEMA_COMPONENTS_INVALID, errorMessage);
                this.logSurfaceLifecycle(
                  'handleMessage failed',
                  `reason=components invalid, dslPreview=${this.createDslPreviewForLog(dsl)}`
                );
                this.onError?.(SurfaceErrorCode.SCHEMA_COMPONENTS_INVALID, errorMessage);
                return;
            }
            if (nativeErrorCode === 'CATALOG_ID_MISSING') {
                this.lastSurfaceCode =
                    this.recordSurfaceResult(SurfaceErrorCode.SCHEMA_CATALOG_ID_MISSING, errorMessage);
                this.logSurfaceLifecycle(
                  'handleMessage failed',
                  `reason=catalogId missing, dslPreview=${this.createDslPreviewForLog(dsl)}`
                );
                this.onError?.(SurfaceErrorCode.SCHEMA_CATALOG_ID_MISSING, errorMessage);
                return;
            }
            if (this.isSurfacePolicyCode(processResult.surfaceResultCode)) {
                let surfacePolicyCode = processResult.surfaceResultCode as SurfaceErrorCode;
                this.lastSurfaceCode = this.recordSurfaceResult(surfacePolicyCode, errorMessage);
                this.emitSchemaWarningsIfNeeded();
                this.reportSurfacePolicyFailure(this.lastSurfaceCode, this.lastSurfaceMessage);
                return;
            }
            this.lastSurfaceCode = this.recordSurfaceResult(SurfaceErrorCode.NATIVE_PROCESS_FAILED, errorMessage);
            this.logSurfaceLifecycle(
              'handleMessage failed',
              `reason=native process failed, errorCode=${nativeErrorCode}, errorMessage=${errorMessage}`
            );
            this.emitSchemaWarningsIfNeeded();
            this.onError?.(SurfaceErrorCode.NATIVE_PROCESS_FAILED, errorMessage);
            return;
        }

        CrossLanguageAttributeBridge.flushPending(this.renderId);

        if (processResult.surfaceId === undefined || processResult.messageType === undefined) {
            this.lastSurfaceCode = this.recordSurfaceResult(SurfaceErrorCode.NATIVE_PROCESS_FAILED, 'native message metadata missing');
            this.logSurfaceLifecycle('handleMessage failed', 'reason=native message metadata missing');
            this.emitSchemaWarningsIfNeeded();
            this.onError?.(SurfaceErrorCode.NATIVE_PROCESS_FAILED, 'native message metadata missing');
            return;
        }

        let surfaceId = processResult.surfaceId;
        let messageType = processResult.messageType as A2UIMessageType;
        this.applySurfaceLifecycle(surfaceId, messageType);
        this.lastSurfaceCode = this.recordSurfaceResult(SurfaceErrorCode.NO_ERROR);
        this.logSurfaceLifecycle(
          'handleMessage success',
          `messageType=${SurfaceControllerImpl.describeMessageTypeForLog(messageType)}, surfaceId=${surfaceId}`
        );
        this.emitSchemaWarningsIfNeeded();
        this.notifyHandlingResult(messageType, surfaceId);
    }

    public enableSchemaWarningReport(enableSchemaWarningReport: boolean): void {
      if (this.destroyed) {
        this.logSurfaceLifecycle('enableSchemaWarningReport ignored', 'reason=controller destroyed');
        return;
      }
      this.checkSchema = enableSchemaWarningReport;
      SchemaErrorInfoManager.getInstance().registerWarningGate(this.renderId, this.checkSchema);
      this.logSurfaceLifecycle('schema warning report updated', `enableSchemaWarningReport=${this.checkSchema}`);
    }

    public registerActionReceiver(onAction: ActionReceiver): void {
        if (this.destroyed) {
            console.warn(`SurfaceControllerImpl registerActionReceiver ignored: controller destroyed, renderId=${this.renderId}`);
            return;
        }
        this.onAction = onAction;
    }

    public registerErrorCallback(onError: ErrorCallback): void {
        if (this.destroyed) {
            console.warn(`SurfaceControllerImpl registerErrorCallback ignored: controller destroyed, renderId=${this.renderId}`);
            return;
        }
        this.onError = onError;
    }

    public reportSchemaWarning(schemaWarningInfo: SchemaWarningInfo): void {
      if (this.destroyed) {
        console.warn(`SurfaceController reportSchemaWarning ignored: controller destroyed, renderId=${this.renderId}`);
        return;
      }
      if (schemaWarningInfo === undefined || schemaWarningInfo === null) {
        console.warn(`SurfaceController reportSchemaWarning ignored: invalid warning, renderId=${this.renderId}`);
        return;
      }
      const errorMsg: string = schemaWarningInfo.errorMsg;
      if (typeof errorMsg !== 'string' || errorMsg.length === 0) {
        console.warn(`SurfaceController reportSchemaWarning ignored: invalid warning, renderId=${this.renderId}`);
        return;
      }
      if (!this.checkSchema) {
        return;
      }
      this.logSurfaceLifecycle('custom schema warning reported', `code=${schemaWarningInfo.code}, message=${errorMsg}`);
      this.enqueueReportedSchemaWarning({
        code: schemaWarningInfo.code,
        errorMsg: errorMsg
      });
    }

    public bindToRender(rootSlot: NodeContent): void {
        if (this.destroyed) {
            this.logSurfaceLifecycle('bindToRender ignored', 'reason=controller destroyed');
            return;
        }
        if (!this.apiSupported) {
            this.logSurfaceLifecycle('bindToRender ignored', 'reason=unsupported api version');
            return;
        }
        if (this.currentRootSlot === rootSlot) {
            this.logSurfaceLifecycle('bindToRender ignored', 'reason=same root slot');
            return;
        }
        if (this.currentRootSlot !== undefined) {
            this.logSurfaceLifecycle('rebindToRender');
        }
        this.currentRootSlot = rootSlot;
        this.logSurfaceLifecycle('bindToRender');
        // The native layer resolves surfaces by renderId. Re-binding simply points
        // the same controller-owned render slot at a new host component.
        NativeEngineBridge.bindSurfaceToRender(this.renderId, rootSlot);
    }

    public unbindFromRender(rootSlot: NodeContent): void {
      if (this.destroyed) {
        this.logSurfaceLifecycle('unbindFromRender ignored', 'reason=controller destroyed');
        return;
      }
      if (!this.apiSupported) {
        this.logSurfaceLifecycle('unbindFromRender ignored', 'reason=unsupported api version');
        return;
      }
      // Ignore stale unbind requests so that RenderA disappearing after RenderB
      // has already rebound the controller will not clear RenderB's ownership.
      if (this.currentRootSlot !== rootSlot) {
        this.logSurfaceLifecycle('unbindFromRender ignored', 'reason=stale root slot');
        return;
      }
      this.currentRootSlot = undefined;
      NativeEngineBridge.unbindSurfaceFromRender(this.renderId);
      this.logSurfaceLifecycle('unbindFromRender');
    }

    public getBackGestureEnabled(): boolean {
      return false;
    }

    public registerBackGestureEnabledListener(listener: BackGestureEnabledListener): void {
    }

    public unregisterBackGestureEnabledListener(listener: BackGestureEnabledListener): void {
    }

    public canPop(): boolean {
      return false;
    }

    /**
       * Go back to the previous surface
       * Removes the current (latest) surface and displays the previous one
    */
    public pop(): SurfaceErrorCode {
      this.logSurfaceLifecycle('pop requested');
      let code = this.recordSurfaceResult(SurfaceErrorCode.MULTI_SURFACE_EMPTY_STACK,
        'pop is not supported on a single-surface controller');
      this.logSurfaceLifecycle('pop finished', `code=${code}, message=${this.lastSurfaceMessage}`);
      return code;
    }
    /**
     * Get the current latest surface ID
     * @return The latest surface ID, or empty string if no surface exists
     */
    public getLatestSurfaceId(): string {
        if (this.destroyed) {
            this.logSurfaceLifecycle('getLatestSurfaceId ignored', 'reason=controller destroyed');
            return '';
        }
        return this.getTopSurfaceId();
    }

    public getLastSurfaceErrorCode(): SurfaceErrorCode {
      return this.lastSurfaceCode;
    }

    public getLastSurfaceMessage(): string {
      return this.lastSurfaceMessage;
    }

    public getRenderId(): number {
      return this.renderId;
    }

    public getCatalog(): CatalogImpl {
      return this.catalog;
    }

    public getUIContext(): UIContext {
      return this.uiContext;
    }

    public getApiVersion(): number {
      return this.apiVersion;
    }

    public static getMinSupportedApiVersion(): number {
      return SurfaceControllerImpl.MIN_SUPPORTED_API_VERSION;
    }

    public isApiSupported(): boolean {
      return this.apiSupported;
    }

    public IsFollowSystemColorMode(): boolean {
      return this.isFollowSystemColorMode;
    }

    public destroy(): void {
        if (this.destroyed) {
            return;
        }
        this.logSurfaceLifecycle('destroy start');
        this.destroyed = true;
        SurfaceControllerImpl.controllerByRenderId.delete(this.renderId);
        CrossLanguageAttributeBridge.clearUIContext(this.renderId);
        FunctionBridge.unregisterLocalFunctionErrorListener(this.localFunctionErrorListener);
        this.onAction = undefined;
        this.onError = undefined;
        this.currentRootSlot = undefined;
        this.pendingNativeSchemaWarnings = [];
        this.nativeSchemaWarningFlushScheduled = false;
        this.pendingReportedSchemaWarnings = [];
        this.reportedSchemaWarningFlushScheduled = false;
        let cleanupSurfaceIds = this.getCleanupSurfaceIds();
        cleanupSurfaceIds.forEach((surfaceId: string) => {
          SchemaErrorInfoManager.getInstance().unbindSurface(this.renderId, surfaceId);
          SchemaErrorInfoManager.getInstance().unregisterWarningEmitter(surfaceId);
        });
        SchemaErrorInfoManager.getInstance().unregisterWarningGate(this.renderId);
        console.info(`[SurfaceController] destroy complete, renderId=${this.renderId}`);
        if (this.apiSupported) {
          NativeEngineBridge.destroyRenderSlot(this.renderId);
        }
    }

    private notifyHandlingResult(messageType: A2UIMessageType, surfaceId: string): void {
        if (this.onSurfaceEvent === undefined) {
            return;
        }

        switch (messageType) {
            case A2UIMessageType.CREATE_SURFACE:
                this.onSurfaceEvent?.(SurfaceEventType.SURFACE_CREATED, this, surfaceId);
                return;
            case A2UIMessageType.UPDATE_COMPONENTS:
                this.onSurfaceEvent?.(SurfaceEventType.SURFACE_COMPONENTS_UPDATED, this, surfaceId);
                return;
            case A2UIMessageType.UPDATE_DATA_MODEL:
                this.onSurfaceEvent?.(SurfaceEventType.SURFACE_DATA_MODEL_UPDATED, this, surfaceId);
                return;
            case A2UIMessageType.DELETE_SURFACE:
                this.onSurfaceEvent?.(SurfaceEventType.SURFACE_DELETED, this, surfaceId);
                return;
            default:
                return;
        }
    }

    public setRootFillMode(forceFill: boolean): void {
      if (this.destroyed) {
        return;
      }
      if (!this.apiSupported) {
        this.logSurfaceLifecycle('setRootFillMode ignored', 'reason=unsupported api version');
        return;
      }
      NativeEngineBridge.setRootFillMode(this.renderId, forceFill);
    }

    public setFontSizeScale(scale: number): void {
      if (this.destroyed) {
        return;
      }
      if (!this.apiSupported) {
        this.logSurfaceLifecycle('setFontSizeScale ignored', 'reason=unsupported api version');
        return;
      }
      NativeEngineBridge.setFontSizeScale(this.renderId, scale);
    }

    private innerUpdateThemeMode(mode: ThemeMode): void {
      if (this.destroyed) {
        this.logSurfaceLifecycle('updateThemeMode ignored', 'reason=controller destroyed');
        return;
      }
      // Only update if mode has changed
      if (this.currentThemeMode === mode) {
        this.logSurfaceLifecycle('updateThemeMode skipped', `reason=no change, mode=${ThemeMode[mode]}`);
        return;
      }
      this.logSurfaceLifecycle('updateThemeMode', `oldMode=${ThemeMode[this.currentThemeMode]}, newMode=${ThemeMode[mode]}`);
      this.currentThemeMode = mode;
      NativeEngineBridge.updateThemeMode(this.renderId, mode);
    }
    public updateAndCheckThemeMode(mode: ThemeMode): void {
      if (!this.isFollowSystemColorMode) {
        return;
      }
      this.innerUpdateThemeMode(mode);
    }
    /**
     * Update theme mode (light/dark)
     * @param mode Theme mode (LIGHT or DARK)
     */
    public updateThemeMode(mode: ThemeMode): void {
      this.isFollowSystemColorMode = false;
      this.innerUpdateThemeMode(mode);
    }

    /**
     * Get current theme mode
     * @return Current theme mode (LIGHT or DARK)
     */
    public getThemeMode(): ThemeMode {
      return this.currentThemeMode;
    }

    /**
     * Update active breakpoint.
     * @param breakpoint Breakpoint (XS, SM, MD, LG, or XL)
     */
    public updateBreakpoint(breakpoint: Breakpoint): void {
      if (this.destroyed) {
        this.logSurfaceLifecycle('updateBreakpoint ignored', 'reason=controller destroyed');
        return;
      }

      // Only update if breakpoint has changed
      if (this.currentBreakpoint === breakpoint) {
        this.logSurfaceLifecycle('updateBreakpoint skipped', `reason=no change, breakpoint=${Breakpoint[breakpoint]}`);
        return;
      }

      this.logSurfaceLifecycle('updateBreakpoint', `oldBreakpoint=${Breakpoint[this.currentBreakpoint]}, newBreakpoint=${Breakpoint[breakpoint]}`);
      this.currentBreakpoint = breakpoint;
      NativeEngineBridge.updateBreakpoint(this.renderId, breakpoint);
    }

    public updateBreakpointByWidth(widthVp: number): void {
      if (!Number.isFinite(widthVp) || widthVp <= 0) {
        this.logSurfaceLifecycle(
          'updateBreakpointByWidth skipped',
          `reason=invalid width, widthVp=${widthVp}`
        );
        return;
      }
      this.updateBreakpoint(BreakpointUtils.resolveBreakpoint(widthVp));
    }

    /**
     * Get current breakpoint
     * @return Current breakpoint (XS, SM, MD, LG, or XL)
     */
    public getBreakpoint(): Breakpoint {
      return this.currentBreakpoint;
    }
}
