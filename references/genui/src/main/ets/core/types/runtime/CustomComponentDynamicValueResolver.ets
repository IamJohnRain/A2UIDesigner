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

import nativeEngine, {
  EvaluateDynamicValueResult,
  NativeValue,
  ResolveCustomComponentDynamicValueResult
} from 'liba2ui_native.so';
import { A2UIValueType } from '../../../interface/Types';
import {
  DynamicValueResolver as PublicDynamicValueResolver,
  ResolveValueCallback
} from '../../../interface/DynamicValueResolver';
import { A2UI_BASIC_CATALOG_ID, A2UI_EXTENDED_CATALOG_ID } from '../../base/CapabilitiesCore';
import { A2UIDynamicValue } from '../DynamicValue';
import { ExpressionEngine, ExpressionEvaluateResult } from '../../expression/ExpressionEngine';
import { DynamicValueResolver as RuntimeDynamicValueResolver } from './DynamicValueResolver';
import { A2UIFunctionInvoker, A2UIResolveContext, ResolveResult } from './ResolveContext';

export type CustomComponentResolveCallback<T> = (param: T) => void;

export interface DynamicValueResolverCoreContext {
  customComponentHandle?: number;
  surfaceId?: string;
  componentId?: string;
  renderId?: number;
  catalogId?: string;
  protocolVersion?: string;
  dataModel?: ESObject;
  invokeFunction?: A2UIFunctionInvoker;
}

export class CustomComponentDynamicValueResolver implements PublicDynamicValueResolver {
  private readonly context: DynamicValueResolverCoreContext;

  constructor(context: DynamicValueResolverCoreContext = {}) {
    this.context = context;
  }

  evaluateValue<T>(value: A2UIValueType): T {
    if (value === undefined) {
      return undefined as T;
    }
    const result: ResolveResult<A2UIValueType> = this.resolveValue(value);
    if (result.success) {
      return result.value as T;
    }
    console.warn(`[DynamicValueResolver] evaluateValue failed, reason=${result.errorMessage ?? ''}`);
    return undefined as T;
  }

  resolvePropertyValue<T>(
    key: string,
    path: A2UIValueType,
    callback: ResolveValueCallback<T>
  ): void {
    const customComponentHandle: number = this.context.customComponentHandle ?? -1;
    if (path === undefined) {
      this.clearPropertyValueWithNative(customComponentHandle, key);
      callback(undefined as T);
      return;
    }
    if (this.resolvePropertyValueWithNative(customComponentHandle, key, path, callback)) {
      return;
    }

    const result: ResolveResult<A2UIValueType> =
      this.resolveValue(path);
    if (!result.success) {
      console.warn(
        `[DynamicValueResolver] resolvePropertyValue failed, handle=${customComponentHandle}, key=${key}, ` +
        `reason=${result.errorMessage ?? ''}`
      );
      return;
    }
    callback(result.value as T);
  }

  private resolveValue(value: A2UIValueType): ResolveResult<A2UIValueType> {
    const nativeResult: ResolveResult<A2UIValueType> | undefined = this.resolveValueWithNative(value);
    if (nativeResult !== undefined) {
      return nativeResult;
    }

    if (typeof value === 'string' && CustomComponentDynamicValueResolver.isFullExpressionString(value)) {
      if (!this.shouldAllowExpression()) {
        return {
          success: true,
          source: 'literal',
          value: value
        };
      }
      const expressionResult: ExpressionEvaluateResult = this.evaluateExpression(value);
      if (expressionResult.success) {
        return {
          success: true,
          source: 'literal',
          value: expressionResult.value as A2UIValueType
        };
      }
      return {
        success: false,
        source: 'literal',
        errorMessage: 'expression evaluate failed'
      };
    }

    return RuntimeDynamicValueResolver.resolveValue(value as A2UIDynamicValue, this.buildResolveContext());
  }

  private resolveValueWithNative(value: A2UIValueType): ResolveResult<A2UIValueType> | undefined {
    const renderId: number = this.context.renderId ?? -1;
    const surfaceId: string = this.context.surfaceId ?? '';
    if (!Number.isFinite(renderId) || renderId < 0 || surfaceId.length === 0) {
      return undefined;
    }

    const descriptorJson: string | undefined = JSON.stringify(value);
    if (descriptorJson === undefined) {
      return undefined;
    }

    const engineRecord: Record<string, Object> = nativeEngine as Object as Record<string, Object>;
    if (typeof engineRecord['evaluateDynamicValue'] !== 'function') {
      return undefined;
    }

    try {
      const result: EvaluateDynamicValueResult = nativeEngine.evaluateDynamicValue(
        renderId,
        surfaceId,
        this.context.componentId ?? '',
        descriptorJson,
        this.shouldAllowExpression()
      );
      if (result.success) {
        return {
          success: true,
          source: 'literal',
          value: result.value as A2UIValueType
        };
      }
      console.warn(
        `[DynamicValueResolver] native evaluateValue failed, surfaceId=${surfaceId}, ` +
        `componentId=${this.context.componentId ?? ''}, reason=${result.errorMessage ?? ''}`
      );
    } catch (error) {
      console.warn(
        `[DynamicValueResolver] native evaluateValue exception, surfaceId=${surfaceId}, ` +
        `componentId=${this.context.componentId ?? ''}, reason=${JSON.stringify(error)}`
      );
    }
    return undefined;
  }

  private buildResolveContext(): A2UIResolveContext {
    return {
      renderId: this.context.renderId,
      surfaceId: this.context.surfaceId,
      componentId: this.context.componentId,
      dataModel: this.context.dataModel,
      invokeFunction: this.context.invokeFunction
    };
  }

  private evaluateExpression(value: string): ExpressionEvaluateResult {
    const renderId: number = this.context.renderId ?? -1;
    const surfaceId: string = this.context.surfaceId ?? '';
    return renderId >= 0 && surfaceId.length > 0 ?
      ExpressionEngine.evaluateWithContext(value, renderId, surfaceId) :
      ExpressionEngine.evaluate(value);
  }

  private resolvePropertyValueWithNative<T>(
    customComponentHandle: number,
    key: string,
    path: A2UIValueType,
    callback: CustomComponentResolveCallback<T>
  ): boolean {
    if (!Number.isFinite(customComponentHandle) || customComponentHandle < 0) {
      return false;
    }
    const descriptorJson: string | undefined = JSON.stringify(path);
    if (descriptorJson === undefined) {
      return false;
    }

    try {
      const result: ResolveCustomComponentDynamicValueResult =
        nativeEngine.resolveCustomComponentDynamicValue(
          customComponentHandle,
          key,
          descriptorJson,
          (value: NativeValue): void => {
            callback(value as T);
          }
        );
      if (result.success) {
        return true;
      }
      console.warn(
        `[DynamicValueResolver] native resolvePropertyValue failed, handle=${customComponentHandle}, key=${key}, ` +
        `reason=${result.errorMessage ?? ''}`
      );
    } catch (error) {
      console.warn(
        `[DynamicValueResolver] native resolvePropertyValue exception, handle=${customComponentHandle}, key=${key}, ` +
        `reason=${JSON.stringify(error)}`
      );
    }
    return false;
  }

  private clearPropertyValueWithNative(customComponentHandle: number, key: string): void {
    if (!Number.isFinite(customComponentHandle) || customComponentHandle < 0 || key.length === 0) {
      return;
    }
    if (typeof nativeEngine.clearCustomComponentDynamicValue !== 'function') {
      return;
    }
    try {
      nativeEngine.clearCustomComponentDynamicValue(customComponentHandle, key);
    } catch (error) {
      console.warn(
        `[DynamicValueResolver] native clearPropertyValue exception, handle=${customComponentHandle}, key=${key}, ` +
        `reason=${JSON.stringify(error)}`
      );
    }
  }

  private shouldAllowExpression(): boolean {
    if (this.isExtendedProtocolContext()) {
      return true;
    }
    return !this.isBasicProtocolContext();
  }

  private isExtendedProtocolContext(): boolean {
    const catalogId: string = this.context.catalogId ?? '';
    const protocolVersion: string = this.context.protocolVersion ?? '';
    return catalogId.toLowerCase() === A2UI_EXTENDED_CATALOG_ID.toLowerCase() ||
      protocolVersion.trim() === 'v1.0.0';
  }

  private isBasicProtocolContext(): boolean {
    const catalogId: string = this.context.catalogId ?? '';
    const protocolVersion: string = this.context.protocolVersion ?? '';
    return catalogId.toLowerCase() === A2UI_BASIC_CATALOG_ID.toLowerCase() ||
      protocolVersion.trim() === 'v0.9';
  }

  private static isFullExpressionString(value: string): boolean {
    const trimmed: string = value.trim();
    return trimmed.startsWith('{{') && trimmed.endsWith('}}');
  }
}
