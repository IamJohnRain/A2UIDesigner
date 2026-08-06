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

import { A2UIValueType } from '../../types/types';
import { A2UIDataBinding } from '../DataBinding';
import { A2UIDynamicValue } from '../DynamicValue';
import { A2UIDynamicString } from '../DynamicString';
import { A2UIDynamicNumber } from '../DynamicNumber';
import { A2UIDynamicBoolean } from '../DynamicBoolean';
import { A2UIDynamicStringList } from '../DynamicStringList';
import { A2UIFunctionArgs, A2UIFunctionCall } from '../FunctionCall';
import { A2UIResolveContext, ResolveResult } from './ResolveContext';
import { TypeGuards } from './TypeGuards';
import { ExpressionEngine, ExpressionEvaluateResult } from '../../expression/ExpressionEngine';

const MAX_DYNAMIC_RESOLVE_DEPTH: number = 16;

function okResult<T>(value: T, source: 'literal' | 'binding' | 'function' | 'expression'): ResolveResult<T> {
  return {
    success: true,
    source: source,
    value: value
  };
}

function failResult<T>(
  source: 'literal' | 'binding' | 'function' | 'expression',
  errorMessage: string
): ResolveResult<T> {
  return {
    success: false,
    source: source,
    errorMessage: errorMessage
  };
}

function isFullExpressionString(value: string): boolean {
  let trimmed: string = value.trim();
  return trimmed.startsWith('{{') && trimmed.endsWith('}}');
}

export class DynamicValueResolver {
  static resolveValue(value: A2UIDynamicValue | undefined, context?: A2UIResolveContext): ResolveResult<A2UIValueType> {
    let result: ResolveResult<A2UIValueType> = DynamicValueResolver.resolveValueRecursive(value, context, 0);
    if (!result.success && context?.reportError !== undefined) {
      context.reportError(result.source, result.errorMessage ?? 'dynamic value resolve failed');
    }
    return result;
  }

  static resolveString(value: A2UIDynamicString | undefined, defaultValue: string = '',
    context?: A2UIResolveContext): string {
    let result = DynamicValueResolver.resolveValue(value, context);
    return result.success && typeof result.value === 'string' ? result.value : defaultValue;
  }

  static resolveNumber(value: A2UIDynamicNumber | undefined, defaultValue: number,
    context?: A2UIResolveContext): number {
    let result = DynamicValueResolver.resolveValue(value, context);
    return result.success && typeof result.value === 'number' && Number.isFinite(result.value)
      ? result.value
      : defaultValue;
  }

  static resolveBoolean(value: A2UIDynamicBoolean | undefined, defaultValue: boolean = false,
    context?: A2UIResolveContext): boolean {
    let result = DynamicValueResolver.resolveValue(value, context);
    return result.success && typeof result.value === 'boolean' ? result.value : defaultValue;
  }

  static resolveStringList(value: A2UIDynamicStringList | undefined, defaultValue: string[] = [],
    context?: A2UIResolveContext): string[] {
    let result = DynamicValueResolver.resolveValue(value, context);
    if (!result.success || !TypeGuards.isStringArray(result.value)) {
      return defaultValue.slice();
    }
    return (result.value as string[]).slice();
  }

  private static resolveBinding(binding: A2UIDataBinding,
    context?: A2UIResolveContext): ResolveResult<A2UIValueType> {
    if (context?.dataModel === undefined) {
      return failResult<A2UIValueType>('binding', `data model is unavailable for path: ${binding.path}`);
    }

    let resolved = DynamicValueResolver.readPath(context.dataModel, binding.path);
    if (resolved === undefined) {
      return failResult<A2UIValueType>('binding', `binding path not found: ${binding.path}`);
    }
    return okResult<A2UIValueType>(resolved, 'binding');
  }

  private static resolveFunctionCall(
    functionCall: A2UIFunctionCall,
    context?: A2UIResolveContext,
    depth: number = 0
  ): ResolveResult<A2UIValueType> {
    if (context?.invokeFunction === undefined) {
      return failResult<A2UIValueType>('function', `function invoker is unavailable: ${functionCall.call}`);
    }

    let resolvedCall: A2UIFunctionCall = {
      call: functionCall.call,
      returnType: functionCall.returnType,
      args: DynamicValueResolver.resolveFunctionArgs(functionCall.args, context, depth)
    };

    try {
      let result = context.invokeFunction(resolvedCall);
      return okResult<A2UIValueType>(result, 'function');
    } catch (error) {
      return failResult<A2UIValueType>('function', `function execution failed: ${JSON.stringify(error)}`);
    }
  }

  private static resolveFunctionArgs(
    args: A2UIFunctionArgs | undefined,
    context?: A2UIResolveContext,
    depth: number = 0
  ): Record<string, A2UIValueType | Record<string, Object>> | undefined {
    if (args === undefined) {
      return undefined;
    }

    let resolved: Record<string, A2UIValueType | Record<string, Object>> = {};
    Object.keys(args).forEach((key: string) => {
      let current = args[key];
      let result = DynamicValueResolver.resolveValueRecursive(current as A2UIDynamicValue, context, depth + 1);
      resolved[key] = result.success ? result.value : undefined;
    });
    return resolved;
  }

  private static resolveValueRecursive(
    value: A2UIDynamicValue | undefined,
    context?: A2UIResolveContext,
    depth: number = 0
  ): ResolveResult<A2UIValueType> {
    if (depth > MAX_DYNAMIC_RESOLVE_DEPTH) {
      return failResult<A2UIValueType>('literal', 'dynamic value resolve depth exceeds limit');
    }
    if (value === undefined) {
      return failResult<A2UIValueType>('literal', 'value is undefined');
    }
    if (TypeGuards.isDataBinding(value)) {
      return DynamicValueResolver.resolveBinding(value as A2UIDataBinding, context);
    }
    if (TypeGuards.isFunctionCall(value)) {
      return DynamicValueResolver.resolveFunctionCall(value as A2UIFunctionCall, context, depth + 1);
    }
    if (typeof value === 'string' && isFullExpressionString(value)) {
      return DynamicValueResolver.resolveExpression(value, context);
    }
    if (Array.isArray(value)) {
      let resolvedItems: A2UIValueType[] = [];
      let items = value as Object[];
      for (let index: number = 0; index < items.length; index++) {
        let result: ResolveResult<A2UIValueType> =
          DynamicValueResolver.resolveValueRecursive(items[index] as A2UIDynamicValue, context, depth + 1);
        if (!result.success) {
          return result;
        }
        resolvedItems.push(result.value);
      }
      return okResult<A2UIValueType>(resolvedItems, 'literal');
    }
    if (TypeGuards.isRecord(value)) {
      let record: Record<string, Object> = value as Record<string, Object>;
      let resolvedRecord: Record<string, Object> = {};
      let keys: string[] = Object.keys(record);
      for (let index: number = 0; index < keys.length; index++) {
        let key: string = keys[index];
        let result: ResolveResult<A2UIValueType> =
          DynamicValueResolver.resolveValueRecursive(record[key] as A2UIDynamicValue, context, depth + 1);
        if (!result.success) {
          return result;
        }
        if (result.value !== undefined) {
          resolvedRecord[key] = result.value as Object;
        }
      }
      return okResult<A2UIValueType>(resolvedRecord, 'literal');
    }
    return okResult<A2UIValueType>(value as A2UIValueType, 'literal');
  }

  private static resolveExpression(
    value: string,
    context?: A2UIResolveContext
  ): ResolveResult<A2UIValueType> {
    let renderId: number = context?.renderId ?? -1;
    let surfaceId: string = context?.surfaceId ?? '';
    let result: ExpressionEvaluateResult = renderId >= 0 && surfaceId.length > 0 ?
      ExpressionEngine.evaluateWithContext(value, renderId, surfaceId) :
      ExpressionEngine.evaluate(value);
    if (!result.success) {
      return failResult<A2UIValueType>('expression', 'expression evaluate failed');
    }
    return okResult<A2UIValueType>(result.value as A2UIValueType, 'expression');
  }

  private static readPath(dataModel: Object, path: string): A2UIValueType | undefined {
    if (path.length === 0 || path === '/') {
      return dataModel as Object as A2UIValueType;
    }

    let normalizedPath: string = path.startsWith('/') ? path.substring(1) : path;
    if (normalizedPath.length === 0) {
      return dataModel as Object as A2UIValueType;
    }

    let current: Object | A2UIValueType = dataModel;
    for (const rawToken of normalizedPath.split('/')) {
      let token = rawToken.replace(/~1/g, '/').replace(/~0/g, '~');
      if (Array.isArray(current)) {
        let index: number = Number(token);
        if (!Number.isInteger(index) || index < 0 || index >= current.length) {
          return undefined;
        }
        current = current[index];
        continue;
      }

      if (!TypeGuards.isRecord(current)) {
        return undefined;
      }

      let record: Record<string, Object> = current as Record<string, Object>;
      current = record[token] as A2UIValueType | Object;
      if (current === undefined) {
        return undefined;
      }
    }

    return current as A2UIValueType;
  }
}
