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

export enum SchemaErrorCode {
  REQUIRED_MISS = 'ERROR_CODE_REQUIRED_MISS',
  INVALID_VALUE = 'ERROR_CODE_INVALID_VALUE',
  UNDEFINED_FIELD = 'ERROR_CODE_UNDEFINED_FIELD',
  TYPE_MISMATCH = 'ERROR_CODE_TYPE_MISMATCH',
  SCHEMA_PARSE_FAILED = 'ERROR_CODE_SCHEMA_PARSE_FAILED',
  FUNCTION_SCHEMA_INVALID = 'ERROR_CODE_FUNCTION_SCHEMA_INVALID'
}

export type SchemaItemType = 'message' | 'component' | 'function' | 'schema';

export interface SchemaErrorInfo {
  code: string;
  message: string;
  path?: string;
  itemType?: SchemaItemType;
  itemName?: string;
  timestamp: string;
}

interface SchemaWarningContext {
  renderId?: number;
  surfaceId?: string;
}

interface SchemaWarningCaptureState {
  warnings: SchemaErrorInfo[];
  mirrorToGlobal: boolean;
}

export interface CapturedSchemaWarnings<T> {
  result: T;
  warnings: SchemaErrorInfo[];
}

export class SchemaErrorInfoManager {
  private static instance: SchemaErrorInfoManager = new SchemaErrorInfoManager();

  private warnings: SchemaErrorInfo[] = [];
  private schemaWarningEnabledByRenderId: Map<number, boolean> = new Map<number, boolean>();
  private surfaceIdToRenderIds: Map<string, number[]> = new Map<string, number[]>();
  private surfaceWarningEmitters: Map<string, (warnings: SchemaErrorInfo[]) => void> =
    new Map<string, (warnings: SchemaErrorInfo[]) => void>();
  private warningContextStack: SchemaWarningContext[] = [];
  private warningCaptureStack: SchemaWarningCaptureState[] = [];
  private messageWarningCollectionDepth: number = 0;

  private constructor() {
  }

  public static getInstance(): SchemaErrorInfoManager {
    return SchemaErrorInfoManager.instance;
  }

  public clear(): void {
    this.warnings = [];
  }

  public registerWarningGate(renderId: number, enabled: boolean): void {
    this.schemaWarningEnabledByRenderId.set(renderId, enabled);
  }

  public unregisterWarningGate(renderId: number): void {
    this.schemaWarningEnabledByRenderId.delete(renderId);
    this.surfaceIdToRenderIds.forEach((renderIds: number[], surfaceId: string) => {
      let filtered: number[] = renderIds.filter((currentRenderId: number) => currentRenderId !== renderId);
      if (filtered.length === 0) {
        this.surfaceIdToRenderIds.delete(surfaceId);
        return;
      }
      this.surfaceIdToRenderIds.set(surfaceId, filtered);
    });
  }

  public bindSurface(renderId: number, surfaceId: string): void {
    if (surfaceId.length === 0) {
      return;
    }
    let renderIds: number[] = this.surfaceIdToRenderIds.get(surfaceId) ?? [];
    let filtered: number[] = renderIds.filter((currentRenderId: number) => currentRenderId !== renderId);
    filtered.push(renderId);
    this.surfaceIdToRenderIds.set(surfaceId, filtered);
  }

  public unbindSurface(renderId: number, surfaceId: string): void {
    if (surfaceId.length === 0) {
      return;
    }
    let renderIds: number[] | undefined = this.surfaceIdToRenderIds.get(surfaceId);
    if (renderIds === undefined) {
      return;
    }
    let filtered: number[] = renderIds.filter((currentRenderId: number) => currentRenderId !== renderId);
    if (filtered.length === 0) {
      this.surfaceIdToRenderIds.delete(surfaceId);
      return;
    }
    this.surfaceIdToRenderIds.set(surfaceId, filtered);
  }

  public registerWarningEmitter(surfaceId: string, emitter: (warnings: SchemaErrorInfo[]) => void): void {
    if (surfaceId.length === 0) {
      return;
    }
    this.surfaceWarningEmitters.set(surfaceId, emitter);
  }

  public unregisterWarningEmitter(surfaceId: string): void {
    if (surfaceId.length === 0) {
      return;
    }
    this.surfaceWarningEmitters.delete(surfaceId);
  }

  public withSchemaWarningContext<T>(
    renderId: number | undefined,
    surfaceId: string | undefined,
    task: () => T
  ): T {
    this.warningContextStack.push({ renderId: renderId, surfaceId: surfaceId });
    try {
      return task();
    } finally {
      this.warningContextStack.pop();
    }
  }

  public withMessageWarningCollection<T>(task: () => T): T {
    this.messageWarningCollectionDepth += 1;
    try {
      return task();
    } finally {
      this.messageWarningCollectionDepth -= 1;
    }
  }

  public isCollectingWarningsForMessage(): boolean {
    return this.messageWarningCollectionDepth > 0;
  }

  public captureWarnings<T>(task: () => T, mirrorToGlobal: boolean = false): CapturedSchemaWarnings<T> {
    const captureState: SchemaWarningCaptureState = {
      warnings: [],
      mirrorToGlobal: mirrorToGlobal
    };
    this.warningCaptureStack.push(captureState);
    let result: T | undefined = undefined;
    try {
      result = task();
    } finally {
      this.warningCaptureStack.pop();
    }
    return {
      result: result as T,
      warnings: captureState.warnings.map((warning: SchemaErrorInfo): SchemaErrorInfo => this.copyWarning(warning))
    };
  }

  public dispatchCapturedWarnings(surfaceId: string | undefined, warnings: SchemaErrorInfo[]): void {
    if (surfaceId === undefined || surfaceId.length === 0 || warnings.length === 0) {
      return;
    }
    let emitter = this.surfaceWarningEmitters.get(surfaceId);
    if (emitter === undefined) {
      return;
    }
    emitter(warnings.map((warning: SchemaErrorInfo): SchemaErrorInfo => this.copyWarning(warning)));
  }

  public isSchemaWarningEnabled(renderId?: number, surfaceId?: string): boolean {
    let resolvedRenderId: number | undefined = this.resolveRenderId(renderId, surfaceId);
    if (resolvedRenderId === undefined) {
      return true;
    }
    return this.schemaWarningEnabledByRenderId.get(resolvedRenderId) ?? true;
  }

  public recordSchemaWarning(
    code: SchemaErrorCode | string,
    message: string,
    path?: string,
    itemType?: SchemaItemType,
    itemName?: string
  ): void {
    let currentContext: SchemaWarningContext | undefined = this.getCurrentWarningContext();
    if (!this.isSchemaWarningEnabled(currentContext?.renderId, currentContext?.surfaceId)) {
      return;
    }
    const warning: SchemaErrorInfo = {
      code: code,
      message: message,
      path: path,
      itemType: itemType,
      itemName: itemName,
      timestamp: new Date().toISOString()
    };
    let captureState: SchemaWarningCaptureState | undefined = this.getCurrentWarningCapture();
    if (captureState !== undefined) {
      captureState.warnings.push(this.copyWarning(warning));
    }
    if (captureState === undefined || captureState.mirrorToGlobal) {
      this.warnings.push(warning);
    }

    let logMessage = `[SchemaError] ${code}: ${message}`;
    if (path !== undefined && path.length > 0) {
      logMessage += `, path=${path}`;
    }
    if (itemType !== undefined && itemName !== undefined) {
      logMessage += `, item=${itemType}:${itemName}`;
    }
    console.warn(logMessage);
  }

  public getWarnings(): SchemaErrorInfo[] {
    let copiedWarnings: SchemaErrorInfo[] = [];
    this.warnings.forEach((warning: SchemaErrorInfo) => {
      copiedWarnings.push(this.copyWarning(warning));
    });
    return copiedWarnings;
  }

  public getWarningCount(): number {
    return this.warnings.length;
  }

  public hasWarnings(): boolean {
    return this.warnings.length > 0;
  }

  private getCurrentWarningContext(): SchemaWarningContext | undefined {
    if (this.warningContextStack.length === 0) {
      return undefined;
    }
    return this.warningContextStack[this.warningContextStack.length - 1];
  }

  private getCurrentWarningCapture(): SchemaWarningCaptureState | undefined {
    if (this.warningCaptureStack.length === 0) {
      return undefined;
    }
    return this.warningCaptureStack[this.warningCaptureStack.length - 1];
  }

  private copyWarning(warning: SchemaErrorInfo): SchemaErrorInfo {
    return {
      code: warning.code,
      message: warning.message,
      path: warning.path,
      itemType: warning.itemType,
      itemName: warning.itemName,
      timestamp: warning.timestamp
    };
  }

  private resolveRenderId(renderId?: number, surfaceId?: string): number | undefined {
    if (renderId !== undefined && this.schemaWarningEnabledByRenderId.has(renderId)) {
      return renderId;
    }
    if (surfaceId === undefined || surfaceId.length === 0) {
      return undefined;
    }
    let renderIds: number[] | undefined = this.surfaceIdToRenderIds.get(surfaceId);
    if (renderIds === undefined || renderIds.length === 0) {
      return undefined;
    }
    return renderIds[renderIds.length - 1];
  }
}

export const SchemaErrorInfoMananger = SchemaErrorInfoManager;
