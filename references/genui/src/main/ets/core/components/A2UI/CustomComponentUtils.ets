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

import { ComponentContent, UIContext } from '@kit.ArkUI';
import { CustomComponentAttribute } from './CustomComponent';
import { CustomComponentFactory } from './CustomComponentFactory';
import { FunctionBridge } from '../../functions/FunctionBridge';
import { NativeEngineBridge } from '../../base/NativeEngineBridge';
import { SchemaErrorInfoManager } from '../../schema/SchemaErrorInfoManager';
import { A2UIDynamicNumber } from '../../types/DynamicNumber';
import { A2UIDynamicString } from '../../types/DynamicString';
import { A2UIDynamicValue } from '../../types/DynamicValue';
import { A2UIFunctionCall } from '../../types/FunctionCall';
import { A2UIValueType } from '../../types/types';
import { A2UIResolveContext, ResolveResult } from '../../types/runtime/ResolveContext';
import { DynamicValueResolver } from '../../types/runtime/DynamicValueResolver';
import { ExpressionEngine } from '../../expression/ExpressionEngine';

export interface Margin {
  top?: number;
  right?: number;
  bottom?: number;
  left?: number;
}

export function buildCustomPropsSignature(attribute: CustomComponentAttribute | undefined): string {
  let customPropsSignature: string = 'undefined';
  let customProps: Record<string, Object> | undefined = attribute?.customProps;
  if (customProps !== undefined) {
    try {
      customPropsSignature = JSON.stringify(customProps);
    } catch (_error) {
      customPropsSignature = String(customProps);
    }
  }
  return `${attribute?.renderId ?? -1}|${attribute?.surfaceId ?? ''}|${attribute?.id ?? ''}|${customPropsSignature}`;
}

export interface ResolvedCustomComponentAccessibility {
  label?: string;
  description?: string;
}

export function resolveCustomComponentWarningPathPrefixById(componentId: string | undefined): string {
  const normalizedComponentId: string | undefined = componentId?.trim();
  if (normalizedComponentId !== undefined && normalizedComponentId.length > 0) {
    return normalizedComponentId;
  }
  return 'customProps';
}

export function resolveCustomComponentWarningPathPrefix(
  attribute: CustomComponentAttribute | undefined
): string {
  return resolveCustomComponentWarningPathPrefixById(attribute?.id);
}

export function resolveMargin(attribute: CustomComponentAttribute | undefined): Margin | undefined {
  let marginStr = attribute?.properties?.margin;
  if (marginStr === undefined || marginStr === '') {
    return undefined;
  }
  try {
    let marginObj = JSON.parse(marginStr) as Record<string, number>;
    return {
      top: marginObj['top'] ?? 0,
      right: marginObj['right'] ?? 0,
      bottom: marginObj['bottom'] ?? 0,
      left: marginObj['left'] ?? 0
    };
  } catch (e) {
    return undefined;
  }
}

interface DynamicStringPathOption {
  path: string;
}

interface ExpressionOptionResolveResult {
  success: boolean;
  value?: A2UIValueType;
}

export function parseCustomProps(
  attribute: CustomComponentAttribute | undefined,
  tag: string
): Record<string, Object> | undefined {
  let customProps = attribute?.customProps;
  if (customProps === undefined || customProps === null || Array.isArray(customProps)) {
    return undefined;
  }
  return customProps as Record<string, Object>;
}

function isFullExpressionString(value: string): boolean {
  let trimmed = value.trim();
  return trimmed.startsWith('{{') && trimmed.endsWith('}}');
}

function resolveExpressionOptionValue(
  value: string,
  attribute: CustomComponentAttribute | undefined
): ExpressionOptionResolveResult {
  if (!isFullExpressionString(value)) {
    return { success: false };
  }

  let renderId: number = attribute?.renderId ?? -1;
  let surfaceId: string = attribute?.surfaceId ?? '';
  let result = renderId >= 0 && surfaceId.length > 0 ?
    ExpressionEngine.evaluateWithContext(value, renderId, surfaceId) :
    ExpressionEngine.evaluate(value);
  if (!result.success) {
    return { success: false };
  }
  return {
    success: true,
    value: result.value as A2UIValueType
  };
}

function normalizeNumberOptionValue(value: A2UIValueType | Object | undefined): number | undefined {
  if (typeof value === 'number' && Number.isFinite(value)) {
    return value;
  }
  if (typeof value === 'string') {
    let parsed = Number(value);
    if (!Number.isNaN(parsed) && Number.isFinite(parsed)) {
      return parsed;
    }
  }
  return undefined;
}

export function resolveBooleanOptionValue(value: Object | undefined): boolean | undefined {
  if (typeof value === 'boolean') {
    return value;
  }
  if (typeof value === 'string') {
    if (value === 'true') {
      return true;
    }
    if (value === 'false') {
      return false;
    }
  }
  return undefined;
}

function buildResolveContext(attribute: CustomComponentAttribute | undefined): A2UIResolveContext | undefined {
  if (attribute === undefined) {
    return undefined;
  }
  let surfaceId: string = attribute.surfaceId ?? '';
  let componentId: string = attribute.id ?? '';
  let renderId: number = attribute.renderId ?? -1;
  return {
    renderId: renderId,
    surfaceId: surfaceId,
    componentId: componentId,
    dataModel: attribute.dataModel,
    invokeFunction: (functionCall: A2UIFunctionCall) => FunctionBridge.invoke(functionCall, surfaceId, componentId, renderId)
  } as A2UIResolveContext;
}

export function resolveStringOptionValue(value: Object | undefined, tag: string, field: string): string | undefined {
  if (typeof value === 'string') {
    return value;
  }
  if (typeof value === 'object' && value !== null) {
    let valueObject = value as Record<string, Object>;
    if (typeof valueObject['value'] === 'string') {
      return valueObject['value'];
    }
    if (typeof valueObject['path'] === 'string') {
      console.warn(`${tag}: ${field} is a data binding path, not resolvable in custom component context`);
    }
  }
  return undefined;
}

export function resolveDynamicStringOptionValue(
  value: Object | undefined,
  attribute: CustomComponentAttribute | undefined
): string | undefined {
  if (value === undefined) {
    return undefined;
  }
  if (typeof value === 'string' && isFullExpressionString(value)) {
    let expressionResult: ExpressionOptionResolveResult = resolveExpressionOptionValue(value, attribute);
    return expressionResult.success && typeof expressionResult.value === 'string' &&
      expressionResult.value.length > 0 ? expressionResult.value : undefined;
  }
  let resolveContext: A2UIResolveContext | undefined = buildResolveContext(attribute);
  let resolvedValue: string = DynamicValueResolver.resolveString(value as A2UIDynamicString, '', resolveContext);
  return resolvedValue.length > 0 ? resolvedValue : undefined;
}

export function resolveDynamicNumberOptionValue(
  value: Object | undefined,
  attribute: CustomComponentAttribute | undefined
): number | undefined {
  if (value === undefined) {
    return undefined;
  }
  if (typeof value === 'string' && isFullExpressionString(value)) {
    let expressionResult: ExpressionOptionResolveResult = resolveExpressionOptionValue(value, attribute);
    return expressionResult.success ? normalizeNumberOptionValue(expressionResult.value) : undefined;
  }

  let literalNumber: number | undefined = normalizeNumberOptionValue(value);
  if (literalNumber !== undefined) {
    return literalNumber;
  }

  let resolveContext: A2UIResolveContext | undefined = buildResolveContext(attribute);
  let result: ResolveResult<A2UIValueType> =
    DynamicValueResolver.resolveValue(value as A2UIDynamicNumber, resolveContext);
  return result.success ? normalizeNumberOptionValue(result.value) : undefined;
}

export function resolveDynamicBooleanOptionValue(
  value: Object | undefined,
  attribute: CustomComponentAttribute | undefined
): boolean | undefined {
  let literalValue: boolean | undefined = resolveBooleanOptionValue(value);
  if (literalValue !== undefined) {
    return literalValue;
  }
  if (value === undefined) {
    return undefined;
  }
  if (typeof value === 'string' && isFullExpressionString(value)) {
    let expressionResult: ExpressionOptionResolveResult = resolveExpressionOptionValue(value, attribute);
    return expressionResult.success ?
      resolveBooleanOptionValue(expressionResult.value as Object | undefined) :
      undefined;
  }
  let resolveContext: A2UIResolveContext | undefined = buildResolveContext(attribute);
  let result = DynamicValueResolver.resolveValue(value as A2UIDynamicValue, resolveContext);
  if (!result.success) {
    return undefined;
  }
  if (typeof result.value === 'boolean') {
    return result.value;
  }
  return resolveBooleanOptionValue(result.value as Object | undefined);
}

export function resolveDynamicStringPathOptionValue(
  path: string | undefined,
  attribute: CustomComponentAttribute | undefined
): string | undefined {
  if (path === undefined || path.length === 0) {
    return undefined;
  }
  let pathValue: DynamicStringPathOption = { path: path };
  return resolveDynamicStringOptionValue(pathValue as Object, attribute);
}

export function isSupportedMediaUrl(url: string, supportedExtensions: string[], dataMediaPrefix: string): boolean {
  let trimmedUrl: string = url.trim();
  if (trimmedUrl.length === 0) {
    return false;
  }

  let normalizedUrl: string = trimmedUrl.toLowerCase();
  if (normalizedUrl.startsWith(dataMediaPrefix)) {
    return true;
  }

  let queryIndex: number = normalizedUrl.indexOf('?');
  if (queryIndex >= 0) {
    normalizedUrl = normalizedUrl.substring(0, queryIndex);
  }
  let hashIndex: number = normalizedUrl.indexOf('#');
  if (hashIndex >= 0) {
    normalizedUrl = normalizedUrl.substring(0, hashIndex);
  }

  for (const extension of supportedExtensions) {
    if (normalizedUrl.endsWith(extension)) {
      return true;
    }
  }
  return false;
}

export function withAttributeSchemaWarningContext<T>(
  attribute: CustomComponentAttribute | undefined,
  task: () => T
): T {
  return SchemaErrorInfoManager.getInstance().withSchemaWarningContext(
    attribute?.renderId,
    attribute?.surfaceId,
    task
  );
}

export function parseJsonArrayOptionValue(
  value: Object | undefined,
  tag: string,
  field: string
): Object[] | undefined {
  if (Array.isArray(value)) {
    return value as Object[];
  }

  if (typeof value === 'string') {
    try {
      let parsed = JSON.parse(value) as Object;
      if (Array.isArray(parsed)) {
        return parsed as Object[];
      }
    } catch (error) {
      console.error(`${tag} parse ${field} failed: ${JSON.stringify(error)}`);
    }
  }
  return undefined;
}

export function resolveDialogUIContext(tag: string, fallback: () => UIContext): UIContext | undefined {
  let hostUIContext: UIContext | undefined = CustomComponentFactory.getCurrentUIContext();
  if (hostUIContext !== undefined) {
    return hostUIContext;
  }

  try {
    return fallback();
  } catch (error) {
    console.error(`${tag} resolveDialogUIContext failed: ${JSON.stringify(error)}`);
    return undefined;
  }
}

export function closeCustomDialogSafely<T extends Object>(
  tag: string,
  uiContext: UIContext | null,
  dialogContent: ComponentContent<T> | null
): void {
  if (dialogContent === null || uiContext === null) {
    return;
  }

  try {
    uiContext.getPromptAction().closeCustomDialog(dialogContent).catch((error: Object) => {
      console.error(`${tag} closeCustomDialog failed: ${JSON.stringify(error)}`);
    });
  } catch (error) {
    console.error(`${tag} closeCustomDialog failed: ${JSON.stringify(error)}`);
  }
}

/**
 * Unified event dispatch for all ETS custom components.
 * Dispatches an event (onClick, onAppear, onChange, etc.) to the native EventHandler chain.
 */
export function dispatchComponentEvent(
  attribute: CustomComponentAttribute | undefined,
  eventName: string,
  payload: Record<string, Object> = {}
): void {
  let renderId: number = attribute?.renderId ?? -1;
  let surfaceId: string = attribute?.surfaceId ?? '';
  let componentId: string = attribute?.id ?? '';
  if (renderId < 0 || surfaceId.length === 0 || componentId.length === 0) {
    return;
  }
  NativeEngineBridge.dispatchCustomComponentAction(
    renderId, surfaceId, componentId, eventName, JSON.stringify(payload)
  );
}

export function showCustomDialogSafely<T extends Object>(
  tag: string,
  uiContext: UIContext,
  dialogContent: ComponentContent<T>,
  onDidDisappear: () => void
): void {
  try {
    uiContext.getPromptAction().openCustomDialog(dialogContent, {
      alignment: DialogAlignment.Center,
      autoCancel: true,
      onDidDisappear: () => {
        onDidDisappear();
      }
    }).catch((error: Object) => {
      onDidDisappear();
      console.error(`${tag} openCustomDialog failed: ${JSON.stringify(error)}`);
    });
  } catch (error) {
    onDidDisappear();
    console.error(`${tag} openCustomDialog failed: ${JSON.stringify(error)}`);
  }
}
