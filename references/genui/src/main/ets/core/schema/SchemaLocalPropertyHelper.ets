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

import { A2UIDynamicString } from '../types/DynamicString';

import { SchemaErrorCode, SchemaErrorInfoManager } from './SchemaErrorInfoManager';

const A2UI_INTERNAL_PROPERTY_KEYS: string[] = ['__a2uiBindings', '_a2uiBindings'];

export class SchemaLocalPropertyHelper {
  private constructor() {
  }

  public static normalizeNumberProperty(
    source: Record<string, Object>,
    key: string,
    fallback: number,
    itemName: string,
    pathPrefix: string = 'customProps'
  ): number {
    const rawValue: Object | undefined = source[key];
    if (rawValue === undefined) {
      return fallback;
    }

    if (typeof rawValue === 'number' && Number.isFinite(rawValue)) {
      return rawValue;
    }

    const numericValue: number = Number(rawValue);
    if (!Number.isNaN(numericValue) && Number.isFinite(numericValue)) {
      SchemaLocalPropertyHelper.recordWarning(
        SchemaErrorCode.TYPE_MISMATCH,
        `Property ${key} expects number, coerce current value to ${numericValue}`,
        `${pathPrefix}.${key}`,
        itemName
      );
      return numericValue;
    }

    SchemaLocalPropertyHelper.recordWarning(
      SchemaErrorCode.TYPE_MISMATCH,
      `Property ${key} expects number, fallback to ${fallback}`,
      `${pathPrefix}.${key}`,
      itemName
    );
    return fallback;
  }

  public static normalizeStringProperty(
    source: Record<string, Object>,
    key: string,
    fallback: string,
    itemName: string,
    pathPrefix: string = 'customProps'
  ): string {
    const rawValue: Object | undefined = source[key];
    if (rawValue === undefined) {
      return fallback;
    }

    if (typeof rawValue === 'string' && rawValue.length > 0) {
      return rawValue;
    }

    SchemaLocalPropertyHelper.recordWarning(
      SchemaErrorCode.TYPE_MISMATCH,
      `Property ${key} expects non-empty string, fallback to default value`,
      `${pathPrefix}.${key}`,
      itemName
    );
    return fallback;
  }

  public static normalizePlainStringProperty(
    source: Record<string, Object>,
    key: string,
    fallback: string,
    itemName: string,
    pathPrefix: string = 'customProps',
    required: boolean = false
  ): string {
    const rawValue: Object | undefined = source[key];
    if (rawValue === undefined) {
      if (required) {
        SchemaLocalPropertyHelper.recordWarning(
          SchemaErrorCode.REQUIRED_MISS,
          `Property ${key} is required`,
          `${pathPrefix}.${key}`,
          itemName
        );
      }
      return fallback;
    }

    if (typeof rawValue === 'string') {
      return rawValue;
    }

    if (SchemaLocalPropertyHelper.isRecordValue(rawValue)) {
      let rawRecord: Record<string, Object> = rawValue as Record<string, Object>;
      let objectValue: Object | undefined = rawRecord['value'];
      if (typeof objectValue === 'string') {
        SchemaLocalPropertyHelper.recordWarning(
          SchemaErrorCode.TYPE_MISMATCH,
          `Property ${key} expects string, object.value has been coerced`,
          `${pathPrefix}.${key}`,
          itemName
        );
        return objectValue;
      }
      let bindingPath: Object | undefined = rawRecord['path'];
      if (typeof bindingPath === 'string') {
        SchemaLocalPropertyHelper.recordWarning(
          SchemaErrorCode.INVALID_VALUE,
          `Property ${key} does not support local path binding, fallback to default value`,
          `${pathPrefix}.${key}`,
          itemName
        );
        return fallback;
      }
    }

    SchemaLocalPropertyHelper.recordWarning(
      SchemaErrorCode.TYPE_MISMATCH,
      `Property ${key} expects string, fallback to default value`,
      `${pathPrefix}.${key}`,
      itemName
    );
    return fallback;
  }

  public static normalizeBooleanProperty(
    source: Record<string, Object>,
    key: string,
    fallback: boolean,
    itemName: string,
    pathPrefix: string = 'customProps',
    required: boolean = false
  ): boolean {
    const rawValue: Object | undefined = source[key];
    if (rawValue === undefined) {
      if (required) {
        SchemaLocalPropertyHelper.recordWarning(
          SchemaErrorCode.REQUIRED_MISS,
          `Property ${key} is required`,
          `${pathPrefix}.${key}`,
          itemName
        );
      }
      return fallback;
    }

    if (typeof rawValue === 'boolean') {
      return rawValue;
    }

    if (typeof rawValue === 'string') {
      if (rawValue === 'true') {
        SchemaLocalPropertyHelper.recordWarning(
          SchemaErrorCode.TYPE_MISMATCH,
          `Property ${key} expects boolean, string true has been coerced`,
          `${pathPrefix}.${key}`,
          itemName
        );
        return true;
      }
      if (rawValue === 'false') {
        SchemaLocalPropertyHelper.recordWarning(
          SchemaErrorCode.TYPE_MISMATCH,
          `Property ${key} expects boolean, string false has been coerced`,
          `${pathPrefix}.${key}`,
          itemName
        );
        return false;
      }
    }

    if (SchemaLocalPropertyHelper.isRecordValue(rawValue)) {
      let rawRecord: Record<string, Object> = rawValue as Record<string, Object>;
      let objectValue: Object | undefined = rawRecord['value'];
      if (typeof objectValue === 'boolean') {
        SchemaLocalPropertyHelper.recordWarning(
          SchemaErrorCode.TYPE_MISMATCH,
          `Property ${key} expects boolean, object.value has been coerced`,
          `${pathPrefix}.${key}`,
          itemName
        );
        return objectValue;
      }
      if (typeof objectValue === 'string') {
        if (objectValue === 'true') {
          SchemaLocalPropertyHelper.recordWarning(
            SchemaErrorCode.TYPE_MISMATCH,
            `Property ${key} expects boolean, object.value true has been coerced`,
            `${pathPrefix}.${key}`,
            itemName
          );
          return true;
        }
        if (objectValue === 'false') {
          SchemaLocalPropertyHelper.recordWarning(
            SchemaErrorCode.TYPE_MISMATCH,
            `Property ${key} expects boolean, object.value false has been coerced`,
            `${pathPrefix}.${key}`,
            itemName
          );
          return false;
        }
      }
      let bindingPath: Object | undefined = rawRecord['path'];
      if (typeof bindingPath === 'string') {
        SchemaLocalPropertyHelper.recordWarning(
          SchemaErrorCode.INVALID_VALUE,
          `Property ${key} does not support local path binding, fallback to ${fallback}`,
          `${pathPrefix}.${key}`,
          itemName
        );
        return fallback;
      }
    }

    SchemaLocalPropertyHelper.recordWarning(
      SchemaErrorCode.TYPE_MISMATCH,
      `Property ${key} expects boolean, fallback to ${fallback}`,
      `${pathPrefix}.${key}`,
      itemName
    );
    return fallback;
  }

  public static normalizeObjectArrayProperty(
    source: Record<string, Object>,
    key: string,
    itemName: string,
    pathPrefix: string = 'customProps',
    required: boolean = false,
    minItems: number = 0
  ): Record<string, Object>[] {
    const rawValue: Object | undefined = source[key];
    if (rawValue === undefined) {
      if (required) {
        SchemaLocalPropertyHelper.recordWarning(
          SchemaErrorCode.REQUIRED_MISS,
          `Property ${key} is required`,
          `${pathPrefix}.${key}`,
          itemName
        );
      }
      return [];
    }

    let rawItems: Object[] | undefined = SchemaLocalPropertyHelper.parseArrayProperty(rawValue, key, itemName, pathPrefix);
    if (rawItems === undefined) {
      SchemaLocalPropertyHelper.recordWarning(
        SchemaErrorCode.TYPE_MISMATCH,
        `Property ${key} expects array, fallback to empty array`,
        `${pathPrefix}.${key}`,
        itemName
      );
      return [];
    }

    let normalizedItems: Record<string, Object>[] = [];
    for (let i = 0; i < rawItems.length; i++) {
      const currentValue: Object | undefined = rawItems[i];
      if (SchemaLocalPropertyHelper.isRecordValue(currentValue)) {
        normalizedItems.push(currentValue as Record<string, Object>);
        continue;
      }
      SchemaLocalPropertyHelper.recordWarning(
        SchemaErrorCode.TYPE_MISMATCH,
        `Property ${key}[${i}] expects object item, drop current item`,
        `${pathPrefix}.${key}[${i}]`,
        itemName
      );
    }

    if (minItems > 0 && normalizedItems.length < minItems) {
      SchemaLocalPropertyHelper.recordWarning(
        SchemaErrorCode.REQUIRED_MISS,
        'Array size is smaller than schema minimum',
        `${pathPrefix}.${key}`,
        itemName
      );
    }
    return normalizedItems;
  }

  public static normalizeStringArrayProperty(
    source: Record<string, Object>,
    key: string,
    itemName: string,
    pathPrefix: string = 'customProps',
    required: boolean = false
  ): string[] {
    const rawValue: Object | undefined = source[key];
    if (rawValue === undefined) {
      if (required) {
        SchemaLocalPropertyHelper.recordWarning(
          SchemaErrorCode.REQUIRED_MISS,
          `Property ${key} is required`,
          `${pathPrefix}.${key}`,
          itemName
        );
      }
      return [];
    }

    let rawItems: Object[] | undefined = SchemaLocalPropertyHelper.parseArrayProperty(rawValue, key, itemName, pathPrefix);
    if (rawItems === undefined) {
      SchemaLocalPropertyHelper.recordWarning(
        SchemaErrorCode.TYPE_MISMATCH,
        `Property ${key} expects string array, fallback to empty array`,
        `${pathPrefix}.${key}`,
        itemName
      );
      return [];
    }

    let normalizedItems: string[] = [];
    for (let i = 0; i < rawItems.length; i++) {
      const currentValue: Object | undefined = rawItems[i];
      if (typeof currentValue === 'string') {
        normalizedItems.push(currentValue);
        continue;
      }
      SchemaLocalPropertyHelper.recordWarning(
        SchemaErrorCode.TYPE_MISMATCH,
        `Property ${key}[${i}] expects string item, drop current item`,
        `${pathPrefix}.${key}[${i}]`,
        itemName
      );
    }
    return normalizedItems;
  }

  public static normalizeDynamicStringProperty(
    source: Record<string, Object>,
    key: string,
    fallback: A2UIDynamicString,
    itemName: string,
    pathPrefix: string = 'customProps',
    required: boolean = false
  ): A2UIDynamicString {
    const rawValue: Object | undefined = source[key];
    if (rawValue === undefined) {
      if (required) {
        SchemaLocalPropertyHelper.recordWarning(
          SchemaErrorCode.REQUIRED_MISS,
          `Property ${key} is required`,
          `${pathPrefix}.${key}`,
          itemName
        );
      }
      return fallback;
    }

    if (typeof rawValue === 'string' || SchemaLocalPropertyHelper.isRecordValue(rawValue)) {
      return rawValue as A2UIDynamicString;
    }

    SchemaLocalPropertyHelper.recordWarning(
      SchemaErrorCode.TYPE_MISMATCH,
      `Property ${key} expects DynamicString compatible value, fallback to default value`,
      `${pathPrefix}.${key}`,
      itemName
    );
    return fallback;
  }

  public static normalizeNonEmptyStringProperty(
    source: Record<string, Object>,
    key: string,
    itemName: string,
    pathPrefix: string = 'customProps',
    required: boolean = false
  ): string | undefined {
    const rawValue: Object | undefined = source[key];
    if (rawValue === undefined) {
      if (required) {
        SchemaLocalPropertyHelper.recordWarning(
          SchemaErrorCode.REQUIRED_MISS,
          `Property ${key} is required`,
          `${pathPrefix}.${key}`,
          itemName
        );
      }
      return undefined;
    }

    if (typeof rawValue === 'string' && rawValue.length > 0) {
      return rawValue;
    }

    if (SchemaLocalPropertyHelper.isRecordValue(rawValue)) {
      let rawRecord: Record<string, Object> = rawValue as Record<string, Object>;
      let bindingPath: Object | undefined = rawRecord['path'];
      if (typeof bindingPath === 'string') {
        SchemaLocalPropertyHelper.recordWarning(
          SchemaErrorCode.INVALID_VALUE,
          `Property ${key} does not support local path binding`,
          `${pathPrefix}.${key}`,
          itemName
        );
        return undefined;
      }
    }

    SchemaLocalPropertyHelper.recordWarning(
      typeof rawValue === 'string' ? SchemaErrorCode.INVALID_VALUE : SchemaErrorCode.TYPE_MISMATCH,
      `Property ${key} expects non-empty string`,
      `${pathPrefix}.${key}`,
      itemName
    );
    return undefined;
  }

  public static normalizeEnumStringProperty(
    source: Record<string, Object>,
    key: string,
    fallback: string,
    allowedValues: string[],
    itemName: string,
    pathPrefix: string = 'customProps',
    required: boolean = false,
    aliases: Map<string, string> | undefined = undefined,
    invalidFallback: string | undefined = undefined
  ): string {
    const rawValue: Object | undefined = source[key];
    const resolvedFallback: string = invalidFallback !== undefined ? invalidFallback : fallback;
    if (rawValue === undefined) {
      if (required) {
        SchemaLocalPropertyHelper.recordWarning(
          SchemaErrorCode.REQUIRED_MISS,
          `Property ${key} is required`,
          `${pathPrefix}.${key}`,
          itemName
        );
      }
      return fallback;
    }

    let candidate: string | undefined = undefined;
    let fromObjectValue: boolean = false;
    if (typeof rawValue === 'string') {
      candidate = rawValue;
    } else if (SchemaLocalPropertyHelper.isRecordValue(rawValue)) {
      let rawRecord: Record<string, Object> = rawValue as Record<string, Object>;
      let objectValue: Object | undefined = rawRecord['value'];
      if (typeof objectValue === 'string') {
        candidate = objectValue;
        fromObjectValue = true;
      } else {
        let bindingPath: Object | undefined = rawRecord['path'];
        if (typeof bindingPath === 'string') {
          SchemaLocalPropertyHelper.recordWarning(
            SchemaErrorCode.INVALID_VALUE,
            `Property ${key} does not support local path binding, fallback to ${resolvedFallback}`,
            `${pathPrefix}.${key}`,
            itemName
          );
          return resolvedFallback;
        }
      }
    }

    if (candidate === undefined) {
      SchemaLocalPropertyHelper.recordWarning(
        SchemaErrorCode.TYPE_MISMATCH,
        `Property ${key} expects enum string, fallback to ${resolvedFallback}`,
        `${pathPrefix}.${key}`,
        itemName
      );
      return resolvedFallback;
    }

    if (fromObjectValue) {
      SchemaLocalPropertyHelper.recordWarning(
        SchemaErrorCode.TYPE_MISMATCH,
        `Property ${key} expects enum string, object.value has been coerced`,
        `${pathPrefix}.${key}`,
        itemName
      );
    }

    if (aliases !== undefined && aliases.has(candidate)) {
      let normalizedValue: string | undefined = aliases.get(candidate);
      if (normalizedValue !== undefined) {
        SchemaLocalPropertyHelper.recordWarning(
          SchemaErrorCode.INVALID_VALUE,
          `Property ${key} uses legacy value ${candidate}, normalize to ${normalizedValue}`,
          `${pathPrefix}.${key}`,
          itemName
        );
        candidate = normalizedValue;
      }
    }

    if (allowedValues.includes(candidate)) {
      return candidate;
    }

    SchemaLocalPropertyHelper.recordWarning(
      SchemaErrorCode.INVALID_VALUE,
      `Property ${key} value ${candidate} is unsupported, fallback to ${resolvedFallback}`,
      `${pathPrefix}.${key}`,
      itemName
    );
    return resolvedFallback;
  }

  public static recordUnknownFields(
    source: Record<string, Object>,
    allowedKeys: string[],
    itemName: string,
    pathPrefix: string = 'customProps'
  ): void {
    SchemaLocalPropertyHelper.collectUnknownFieldKeys(source, allowedKeys).forEach((key: string) => {
      SchemaLocalPropertyHelper.recordWarning(
        SchemaErrorCode.UNDEFINED_FIELD,
        `Property ${key} is undefined in local schema helper and has been ignored`,
        `${pathPrefix}.${key}`,
        itemName
      );
    });
  }

  public static collectUnknownFieldKeys(source: Record<string, Object>, allowedKeys: string[]): string[] {
    const allowedKeySet: Set<string> = new Set(allowedKeys);
    let unknownKeys: string[] = [];
    Object.keys(source).forEach((key: string) => {
      if (allowedKeySet.has(key) || SchemaLocalPropertyHelper.isInternalPropertyKey(key)) {
        return;
      }
      unknownKeys.push(key);
    });
    return unknownKeys;
  }

  private static isInternalPropertyKey(key: string): boolean {
    return A2UI_INTERNAL_PROPERTY_KEYS.includes(key);
  }

  private static recordWarning(
    code: SchemaErrorCode,
    message: string,
    path: string,
    itemName: string
  ): void {
    SchemaErrorInfoManager.getInstance().recordSchemaWarning(
      code,
      message,
      path,
      'component',
      itemName
    );
  }

  private static parseArrayProperty(
    rawValue: Object,
    key: string,
    itemName: string,
    pathPrefix: string
  ): Object[] | undefined {
    if (Array.isArray(rawValue)) {
      return rawValue as Object[];
    }

    if (typeof rawValue !== 'string') {
      return undefined;
    }

    try {
      let parsedValue: Object = JSON.parse(rawValue) as Object;
      if (Array.isArray(parsedValue)) {
        SchemaLocalPropertyHelper.recordWarning(
          SchemaErrorCode.TYPE_MISMATCH,
          `Property ${key} expects array, JSON string has been parsed`,
          `${pathPrefix}.${key}`,
          itemName
        );
        return parsedValue as Object[];
      }
    } catch (_error) {
    }
    return undefined;
  }

  private static isRecordValue(value: Object | undefined | null): boolean {
    return value !== null && value !== undefined && typeof value === 'object' && !Array.isArray(value);
  }
}
