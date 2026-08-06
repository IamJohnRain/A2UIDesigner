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

import { SymbolGlyphModifier } from '@kit.ArkUI';
import { CustomComponentAttribute } from '../A2UI/CustomComponent';
import { CustomComponentDefinition } from '../A2UI/CustomComponentFactory';
import { getIconSystemSymbolNames, resolveIconSystemSymbolResourceName } from '../A2UI/CustomIcon';
import {
  parseCustomProps,
  resolveDynamicValueOptionValueWithNative,
  resolveDynamicStringOptionValue,
  parseJsonArrayOptionValue,
  resolveCustomComponentWarningPathPrefix,
  withAttributeSchemaWarningContext,
  dispatchComponentEvent
} from '../A2UI/CustomComponentUtils';
import { ThemeMode } from '../../base/NativeEngineBridge';
import { FunctionBridge } from '../../functions/FunctionBridge';
import { SchemaErrorCode, SchemaErrorInfoManager } from '../../schema/SchemaErrorInfoManager';
import { SchemaResourceLoader } from '../../schema/SchemaResourceLoader';
import { A2UIDynamicValue } from '../../types/DynamicValue';
import { A2UIFunctionCall } from '../../types/FunctionCall';
import { A2UIResolveContext, ResolveResult, ResolveSource } from '../../types/runtime/ResolveContext';
import { DynamicValueResolver } from '../../types/runtime/DynamicValueResolver';
import {
  A2UIValueType,
  SchemaProvider
} from '../../types/types';
import {
  buildExtendedCommonStyleModifier,
  ExtendedCommonStyleOptions,
  resolveExtendedCommonStyleOptions
} from './ExtendedCommonStyleModifier';

const EXTENDED_SELECT_TYPE: string = 'Select';
const ARROW_POSITION_START: string = 'start';
const ARROW_POSITION_END: string = 'end';
const ARROW_POSITION_ALLOWED: string[] = [ARROW_POSITION_END, ARROW_POSITION_START];
const MENU_ALIGN_START: string = 'start';
const MENU_ALIGN_CENTER: string = 'center';
const MENU_ALIGN_END: string = 'end';
const MENU_ALIGN_ALLOWED: string[] = [MENU_ALIGN_START, MENU_ALIGN_CENTER, MENU_ALIGN_END];
const DEFAULT_ARROW_POSITION: string = ARROW_POSITION_END;
const SYMBOL_RENDERING_SINGLE: string = 'single';
const SYMBOL_RENDERING_MULTIPLE_COLOR: string = 'multipleColor';
const SYMBOL_RENDERING_MULTIPLE_OPACITY: string = 'multipleOpacity';
const SYMBOL_EFFECT_NONE: string = 'none';
const SYMBOL_EFFECT_SCALE: string = 'scale';
const SYMBOL_EFFECT_HIERARCHICAL: string = 'hierarchical';
const SYMBOL_ICON_SRC_ALLOWED: string[] = getIconSystemSymbolNames();
const SELECT_HEX_COLOR_PATTERN: RegExp = /^#([0-9a-fA-F]{6}|[0-9a-fA-F]{8})$/;
const SELECT_FONT_WEIGHT_ALLOWED: string[] = ['lighter', 'normal', 'regular', 'medium', 'bold', 'bolder'];
const SELECT_FONT_STYLE_ALLOWED: string[] = ['normal', 'italic'];

const SELECT_FONT_COLOR: string = '#E5000000';
const SELECT_FONT_COLOR_DARK: string = '#E5FFFFFF';
const SELECT_SELECTED_OPTION_BG_COLOR: string = '#33007DFF';
const SELECT_SELECTED_OPTION_BG_COLOR_DARK: string = '#33317AF7';
const SELECT_SELECTED_OPTION_FONT_COLOR: string = '#FF007DFF';
const SELECT_SELECTED_OPTION_FONT_COLOR_DARK: string = '#FF3F97E9';
const SELECT_OPTION_FONT_COLOR: string = '#FF182431';
const SELECT_OPTION_FONT_COLOR_DARK: string = '#DBFFFFFF';
const SELECT_BACKGROUND_COLOR: string = '#0C000000';
const SELECT_BACKGROUND_COLOR_DARK: string = '#19FFFFFF';
const DEFAULT_MENU_ALIGN: string = MENU_ALIGN_START;

interface SelectOptionItem {
  value: string;
  icon?: string;
  symbolIcon?: SelectSymbolIcon;
}

interface SelectSymbolIcon {
  src: string;
  fontSize?: number;
  fontWeight?: number;
  fontColor?: string;
  renderingStrategy?: string;
  effectStrategy?: string;
}

interface SelectFont {
  size?: number;
  weight?: string;
  family?: string;
  style?: string;
}

interface SelectDividerOptions {
  strokeWidth?: number;
  color?: string;
  startMargin?: number;
  endMargin?: number;
}

interface MenuAlignOffset {
  dx: number;
  dy: number;
}

interface MenuAlignConfig {
  alignType: string;
  offset?: MenuAlignOffset;
}

interface ExtendedSelectOptions {
  options: SelectOptionItem[];
  selected: number;
  value: string;
  font: SelectFont;
  fontColor: string;
  selectedOptionBgColor: string;
  selectedOptionFont: SelectFont;
  selectedOptionFontColor: string;
  optionBgColor: string;
  optionFont: SelectFont;
  optionFontColor: string;
  space: number;
  arrowPosition: string;
  menuAlign: MenuAlignConfig;
  optionWidth: number;
  optionHeight: number;
  menuBackgroundColor: string;
  divider: SelectDividerOptions | null;
  dividerSet: boolean;
  dividerNull: boolean;
}

function createDefaultMenuAlign(): MenuAlignConfig {
  let config: MenuAlignConfig = {
    alignType: DEFAULT_MENU_ALIGN
  };
  return config;
}

function createDefaultSelectOptions(): ExtendedSelectOptions {
  let options: ExtendedSelectOptions = {
    options: [],
    selected: -1,
    value: '',
    font: {},
    fontColor: '',
    selectedOptionBgColor: '',
    selectedOptionFont: {},
    selectedOptionFontColor: '',
    optionBgColor: '',
    optionFont: {},
    optionFontColor: '',
    space: 0,
    arrowPosition: DEFAULT_ARROW_POSITION,
    menuAlign: createDefaultMenuAlign(),
    optionWidth: 0,
    optionHeight: 0,
    menuBackgroundColor: '',
    divider: {},
    dividerSet: false,
    dividerNull: false
  };
  return options;
}

function isRecordValue(value: Object | undefined): boolean {
  return value !== undefined && value !== null && typeof value === 'object' && !Array.isArray(value);
}

function isDynamicValueShape(value: Object | undefined): boolean {
  if (!isRecordValue(value)) {
    return false;
  }
  let record: Record<string, Object> = value as Record<string, Object>;
  return record['path'] !== undefined || record['call'] !== undefined;
}

function isFullExpressionString(value: Object | undefined): boolean {
  if (typeof value !== 'string') {
    return false;
  }
  let trimmed: string = value.trim();
  return trimmed.startsWith('{{') && trimmed.endsWith('}}');
}

function isDynamicObjectValueShape(value: Object | undefined): boolean {
  return isDynamicValueShape(value) || isFullExpressionString(value);
}

function buildSelectWarningPath(attribute: CustomComponentAttribute | undefined, path: string): string {
  let prefix: string = resolveCustomComponentWarningPathPrefix(attribute);
  if (path.length === 0) {
    return prefix;
  }
  return `${prefix}.${path}`;
}

function recordSelectSchemaWarning(
  attribute: CustomComponentAttribute | undefined,
  code: SchemaErrorCode,
  path: string,
  message: string
): void {
  SchemaErrorInfoManager.getInstance().recordSchemaWarning(
    code,
    message,
    buildSelectWarningPath(attribute, path),
    'component',
    EXTENDED_SELECT_TYPE
  );
}

function buildSelectResolveContext(attribute: CustomComponentAttribute | undefined): A2UIResolveContext | undefined {
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
    invokeFunction: (functionCall: A2UIFunctionCall): A2UIValueType => {
      return FunctionBridge.invoke(functionCall, surfaceId, componentId, renderId);
    },
    reportError: (source: ResolveSource, errorMessage: string): void => {
      if (source !== 'function') {
        FunctionBridge.reportDynamicValueResolveError(surfaceId, componentId, renderId, errorMessage);
      }
    }
  } as A2UIResolveContext;
}

function resolveSelectDynamicValue(
  value: Object | undefined,
  attribute: CustomComponentAttribute | undefined
): A2UIValueType | undefined {
  if (value === undefined) {
    return undefined;
  }
  let nativeResolved: A2UIValueType | undefined =
    resolveDynamicValueOptionValueWithNative(value, attribute);
  if (nativeResolved !== undefined) {
    return nativeResolved;
  }
  let result: ResolveResult<A2UIValueType> =
    DynamicValueResolver.resolveValue(value as A2UIDynamicValue, buildSelectResolveContext(attribute));
  if (!result.success) {
    console.warn(`ExtendedSelect dynamic value resolve failed: ${result.errorMessage}`);
    return undefined;
  }
  return result.value;
}

function resolveSelectNumberValue(
  value: Object | undefined,
  attribute: CustomComponentAttribute | undefined
): number | undefined {
  let resolved: A2UIValueType | undefined = resolveSelectDynamicValue(value, attribute);
  if (typeof resolved === 'number' && Number.isFinite(resolved)) {
    return resolved;
  }
  if (typeof resolved === 'string') {
    let parsed: number = Number(resolved);
    if (Number.isFinite(parsed)) {
      return parsed;
    }
  }
  return undefined;
}

function resolveSelectRecordValue(
  value: Object | undefined,
  attribute: CustomComponentAttribute | undefined
): Record<string, Object> | undefined {
  let resolved: A2UIValueType | undefined = resolveSelectDynamicValue(value, attribute);
  if (isRecordValue(resolved as Object | undefined)) {
    return resolved as Record<string, Object>;
  }
  return undefined;
}

function resolveSelectArrayValue(
  value: Object | undefined,
  attribute: CustomComponentAttribute | undefined
): Object[] | undefined {
  let resolved: A2UIValueType | undefined = resolveSelectDynamicValue(value, attribute);
  if (Array.isArray(resolved)) {
    return resolved as Object[];
  }
  if (typeof resolved === 'string') {
    try {
      let parsed: Object = JSON.parse(resolved) as Object;
      if (Array.isArray(parsed)) {
        return parsed as Object[];
      }
    } catch (error) {
      console.error(`ExtendedSelect parse dynamic array failed: ${JSON.stringify(error)}`);
    }
  }
  return undefined;
}

function resolveSelectDfxValue(
  value: Object | undefined,
  attribute: CustomComponentAttribute | undefined,
  forceDynamic: boolean
): A2UIValueType | undefined {
  if (value === undefined) {
    return undefined;
  }
  if (forceDynamic) {
    return value as A2UIValueType;
  }
  if (!isDynamicValueShape(value)) {
    return undefined;
  }
  return resolveSelectDynamicValue(value, attribute);
}

function reportSelectDynamicTypeMismatch(
  attribute: CustomComponentAttribute | undefined,
  path: string,
  expectedType: string
): void {
  recordSelectSchemaWarning(
    attribute,
    SchemaErrorCode.TYPE_MISMATCH,
    path,
    `Property ${path} expects ${expectedType}, fallback/reset has been applied`
  );
}

function reportSelectDynamicInvalidValue(
  attribute: CustomComponentAttribute | undefined,
  path: string,
  reason: string
): void {
  recordSelectSchemaWarning(
    attribute,
    SchemaErrorCode.INVALID_VALUE,
    path,
    `Property ${path} ${reason}, fallback/reset has been applied`
  );
}

function hasKnownObjectMember(record: Record<string, Object>, knownKeys: string[]): boolean {
  for (let index: number = 0; index < knownKeys.length; index++) {
    if (record[knownKeys[index]] !== undefined) {
      return true;
    }
  }
  return false;
}

function reportSelectUnknownObjectMembers(
  attribute: CustomComponentAttribute | undefined,
  path: string,
  record: Record<string, Object>,
  knownKeys: string[]
): void {
  let keys: string[] = Object.keys(record);
  for (let index: number = 0; index < keys.length; index++) {
    let key: string = keys[index];
    if (!knownKeys.includes(key)) {
      let memberPath: string = `${path}.${key}`;
      recordSelectSchemaWarning(
        attribute,
        SchemaErrorCode.UNDEFINED_FIELD,
        memberPath,
        `Property ${memberPath} is undefined and has been ignored`
      );
    }
  }
}

function validateDynamicNumberResult(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined,
  path: string,
  minValue: number | undefined = undefined,
  minExclusive: boolean = false,
  forceDynamic: boolean = false
): void {
  let resolved: A2UIValueType | undefined = resolveSelectDfxValue(value, attribute, forceDynamic);
  if (resolved === undefined || resolved === null) {
    return;
  }
  if (typeof resolved !== 'number' || !Number.isFinite(resolved)) {
    reportSelectDynamicTypeMismatch(attribute, path, 'number');
    return;
  }
  if (minValue === undefined) {
    return;
  }
  if ((minExclusive && resolved <= minValue) || (!minExclusive && resolved < minValue)) {
    reportSelectDynamicInvalidValue(attribute, path, 'is out of range');
  }
}

function validateDynamicStringResult(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined,
  path: string,
  forceDynamic: boolean = false
): string | undefined {
  let resolved: A2UIValueType | undefined = resolveSelectDfxValue(value, attribute, forceDynamic);
  if (resolved === undefined || resolved === null) {
    return undefined;
  }
  if (typeof resolved !== 'string') {
    reportSelectDynamicTypeMismatch(attribute, path, 'string');
    return undefined;
  }
  return resolved;
}

function isValidSelectColorString(value: string): boolean {
  return value.length === 0 || SELECT_HEX_COLOR_PATTERN.test(value);
}

function validateDynamicColorResult(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined,
  path: string,
  forceDynamic: boolean = false
): void {
  let resolved: string | undefined = validateDynamicStringResult(attribute, value, path, forceDynamic);
  if (resolved === undefined) {
    return;
  }
  if (!isValidSelectColorString(resolved)) {
    reportSelectDynamicInvalidValue(attribute, path, 'is invalid color value');
  }
}

function validateDynamicEnumStringResult(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined,
  path: string,
  allowedValues: string[],
  forceDynamic: boolean = false
): void {
  let resolved: string | undefined = validateDynamicStringResult(attribute, value, path, forceDynamic);
  if (resolved === undefined) {
    return;
  }
  if (!allowedValues.includes(resolved.toLowerCase())) {
    reportSelectDynamicInvalidValue(attribute, path, 'is out of enum range');
  }
}

function validateDynamicSymbolIconSrcResult(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined,
  path: string,
  forceDynamic: boolean = false
): void {
  let resolved: string | undefined = validateDynamicStringResult(attribute, value, path, forceDynamic);
  if (resolved === undefined) {
    return;
  }
  if (!SYMBOL_ICON_SRC_ALLOWED.includes(resolved)) {
    reportSelectDynamicInvalidValue(attribute, path, 'is out of enum range');
  }
}

function resolveDynamicRecordForDfx(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined,
  path: string,
  forceDynamic: boolean = false
): Record<string, Object> | undefined {
  let resolved: A2UIValueType | undefined = resolveSelectDfxValue(value, attribute, forceDynamic);
  if (resolved === undefined || resolved === null) {
    return undefined;
  }
  if (isRecordValue(resolved as Object | undefined)) {
    return resolved as Record<string, Object>;
  }
  reportSelectDynamicTypeMismatch(attribute, path, 'object');
  return undefined;
}

function validateDynamicArrayResult(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined,
  path: string,
  forceDynamic: boolean = false
): Object[] | undefined {
  let resolved: A2UIValueType | undefined = resolveSelectDfxValue(value, attribute, forceDynamic);
  if (resolved === undefined || resolved === null) {
    return undefined;
  }
  if (Array.isArray(resolved)) {
    return resolved as Object[];
  }
  if (typeof resolved === 'string') {
    try {
      let parsed: Object = JSON.parse(resolved) as Object;
      if (Array.isArray(parsed)) {
        reportSelectDynamicTypeMismatch(attribute, path, 'array');
        return parsed as Object[];
      }
    } catch (_error) {
      // Fall through to the schema-format type mismatch warning below.
    }
  }
  reportSelectDynamicTypeMismatch(attribute, path, 'array');
  return undefined;
}

function isFiniteNumberValue(value: Object | undefined): boolean {
  return typeof value === 'number' && Number.isFinite(value);
}

function validatePositiveNumberValue(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined,
  path: string,
  requiredPositive: boolean = true
): void {
  if (value === undefined || value === null) {
    return;
  }
  if (isDynamicValueShape(value)) {
    return;
  }
  if (!isFiniteNumberValue(value)) {
    recordSelectSchemaWarning(
      attribute,
      SchemaErrorCode.TYPE_MISMATCH,
      path,
      `Property ${path} expects number, fallback/reset has been applied`
    );
    return;
  }
  let numericValue: number = Number(value);
  if ((requiredPositive && numericValue <= 0) || (!requiredPositive && numericValue < 0)) {
    recordSelectSchemaWarning(
      attribute,
      SchemaErrorCode.INVALID_VALUE,
      path,
      `Property ${path} is out of range, fallback/reset has been applied`
    );
  }
}

function validateFiniteNumberValue(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined,
  path: string
): void {
  if (value === undefined || value === null) {
    return;
  }
  if (isDynamicValueShape(value)) {
    return;
  }
  if (!isFiniteNumberValue(value)) {
    recordSelectSchemaWarning(
      attribute,
      SchemaErrorCode.TYPE_MISMATCH,
      path,
      `Property ${path} expects number, fallback/reset has been applied`
    );
  }
}

function validateSelectedValue(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined
): void {
  if (value === undefined || value === null) {
    return;
  }
  if (isDynamicValueShape(value)) {
    return;
  }
  if (!isFiniteNumberValue(value)) {
    recordSelectSchemaWarning(
      attribute,
      SchemaErrorCode.TYPE_MISMATCH,
      'selected',
      'Property selected expects number, fallback/reset has been applied'
    );
    return;
  }
  let numericValue: number = Number(value);
  if (numericValue < -1) {
    recordSelectSchemaWarning(
      attribute,
      SchemaErrorCode.INVALID_VALUE,
      'selected',
      'Property selected is out of range, fallback/reset has been applied'
    );
  }
}

function validateStaticStringOrDynamicValue(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined,
  path: string,
  required: boolean
): void {
  if (value === undefined || value === null) {
    if (required) {
      recordSelectSchemaWarning(
        attribute,
        SchemaErrorCode.REQUIRED_MISS,
        path,
        `Property ${path} is required, field has been ignored`
      );
    }
    return;
  }
  if (typeof value === 'string') {
    if (required && value.length === 0) {
      recordSelectSchemaWarning(
        attribute,
        SchemaErrorCode.INVALID_VALUE,
        path,
        `Property ${path} expects non-empty string, field has been ignored`
      );
    }
    return;
  }
  if (isDynamicValueShape(value)) {
    return;
  }
  recordSelectSchemaWarning(
    attribute,
    SchemaErrorCode.TYPE_MISMATCH,
    path,
    `Property ${path} expects string, field has been ignored`
  );
}

function validateStaticColorOrDynamicValue(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined,
  path: string
): void {
  if (value === undefined || value === null) {
    return;
  }
  if (isDynamicValueShape(value)) {
    return;
  }
  if (typeof value !== 'string') {
    recordSelectSchemaWarning(
      attribute,
      SchemaErrorCode.TYPE_MISMATCH,
      path,
      `Property ${path} expects color value, fallback/reset has been applied`
    );
    return;
  }
  if (!isValidSelectColorString(value)) {
    recordSelectSchemaWarning(
      attribute,
      SchemaErrorCode.INVALID_VALUE,
      path,
      `Property ${path} is invalid color value, fallback/reset has been applied`
    );
  }
}

function validateSymbolIconSrcValue(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined,
  path: string
): void {
  if (value === undefined || value === null) {
    return;
  }
  if (typeof value === 'string') {
    if (value.length === 0) {
      recordSelectSchemaWarning(
        attribute,
        SchemaErrorCode.INVALID_VALUE,
        path,
        `Property ${path} expects non-empty system icon name, field has been ignored`
      );
      return;
    }
    if (!SYMBOL_ICON_SRC_ALLOWED.includes(value)) {
      recordSelectSchemaWarning(
        attribute,
        SchemaErrorCode.INVALID_VALUE,
        path,
        `Property ${path} is out of enum range, field has been ignored`
      );
    }
    return;
  }
  if (isDynamicValueShape(value)) {
    return;
  }
  recordSelectSchemaWarning(
    attribute,
    SchemaErrorCode.TYPE_MISMATCH,
    path,
    `Property ${path} expects string, field has been ignored`
  );
}

function validateSelectSymbolIconSchema(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined,
  path: string
): void {
  if (value === undefined || value === null) {
    return;
  }
  if (isDynamicObjectValueShape(value)) {
    return;
  }
  if (!isRecordValue(value)) {
    recordSelectSchemaWarning(
      attribute,
      SchemaErrorCode.TYPE_MISMATCH,
      path,
      `Property ${path} expects object, field has been ignored`
    );
    return;
  }
  let symbolIcon: Record<string, Object> = value as Record<string, Object>;
  validateSymbolIconSrcValue(attribute, symbolIcon['src'], `${path}.src`);
  validatePositiveNumberValue(attribute, symbolIcon['fontSize'], `${path}.fontSize`);
  validatePositiveNumberValue(attribute, symbolIcon['fontWeight'], `${path}.fontWeight`);
  validateStaticColorOrDynamicValue(attribute, symbolIcon['fontColor'], `${path}.fontColor`);
  validateEnumStringValue(
    attribute,
    symbolIcon['renderingStrategy'],
    `${path}.renderingStrategy`,
    [SYMBOL_RENDERING_SINGLE, SYMBOL_RENDERING_MULTIPLE_COLOR, SYMBOL_RENDERING_MULTIPLE_OPACITY]
  );
  validateEnumStringValue(
    attribute,
    symbolIcon['effectStrategy'],
    `${path}.effectStrategy`,
    [SYMBOL_EFFECT_NONE, SYMBOL_EFFECT_SCALE, SYMBOL_EFFECT_HIERARCHICAL]
  );
}

function validateEnumStringValue(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined,
  path: string,
  allowedValues: string[]
): void {
  if (value === undefined || value === null || isDynamicValueShape(value)) {
    return;
  }
  if (typeof value !== 'string') {
    recordSelectSchemaWarning(
      attribute,
      SchemaErrorCode.TYPE_MISMATCH,
      path,
      `Property ${path} expects string enum, fallback/reset has been applied`
    );
    return;
  }
  if (!allowedValues.includes(value.toLowerCase())) {
    recordSelectSchemaWarning(
      attribute,
      SchemaErrorCode.INVALID_VALUE,
      path,
      `Property ${path} is out of enum range, fallback/reset has been applied`
    );
  }
}

function parseSelectOptionsForSchema(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined
): Object[] | undefined {
  if (value === undefined || value === null) {
    recordSelectSchemaWarning(
      attribute,
      SchemaErrorCode.REQUIRED_MISS,
      'options',
      'Property options is required, fallback to empty options'
    );
    return undefined;
  }
  if (isDynamicValueShape(value)) {
    return undefined;
  }
  if (Array.isArray(value)) {
    return value as Object[];
  }
  if (typeof value === 'string') {
    try {
      let parsed: Object = JSON.parse(value) as Object;
      if (Array.isArray(parsed)) {
        return parsed as Object[];
      }
    } catch (error) {
      recordSelectSchemaWarning(
        attribute,
        SchemaErrorCode.TYPE_MISMATCH,
        'options',
        `Property options expects array or stringified array, parse failed: ${JSON.stringify(error)}`
      );
      return undefined;
    }
  }
  recordSelectSchemaWarning(
    attribute,
    SchemaErrorCode.TYPE_MISMATCH,
    'options',
    'Property options expects array or stringified array, fallback to empty options'
  );
  return undefined;
}

function validateSelectOptionsArraySchema(
  attribute: CustomComponentAttribute | undefined,
  options: Object[] | undefined
): void {
  if (options === undefined) {
    return;
  }
  for (let index: number = 0; index < options.length; index++) {
    let item: Object = options[index];
    let itemPath: string = `options[${index}]`;
    if (!isRecordValue(item)) {
      recordSelectSchemaWarning(
        attribute,
        SchemaErrorCode.TYPE_MISMATCH,
        itemPath,
        `Property ${itemPath} expects object, item has been ignored`
      );
      continue;
    }
    let itemRecord: Record<string, Object> = item as Record<string, Object>;
    let optionKeys: string[] = ['value', 'icon', 'symbolIcon'];
    reportSelectUnknownObjectMembers(attribute, itemPath, itemRecord, optionKeys);
    validateStaticStringOrDynamicValue(attribute, itemRecord['value'], `${itemPath}.value`, true);
    validateStaticStringOrDynamicValue(attribute, itemRecord['icon'], `${itemPath}.icon`, false);
    validateSelectSymbolIconSchema(attribute, itemRecord['symbolIcon'], `${itemPath}.symbolIcon`);
  }
}

function validateSelectFontSchema(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined,
  path: string
): void {
  if (value === undefined || value === null) {
    return;
  }
  if (isDynamicObjectValueShape(value)) {
    return;
  }
  if (!isRecordValue(value)) {
    recordSelectSchemaWarning(
      attribute,
      SchemaErrorCode.TYPE_MISMATCH,
      path,
      `Property ${path} expects object, fallback/reset has been applied`
    );
    return;
  }
  let font: Record<string, Object> = value as Record<string, Object>;
  let fontKeys: string[] = ['size', 'weight', 'family', 'style'];
  reportSelectUnknownObjectMembers(attribute, path, font, fontKeys);
  if (!hasKnownObjectMember(font, fontKeys)) {
    return;
  }
  validatePositiveNumberValue(attribute, font['size'], `${path}.size`);
  validateEnumStringValue(attribute, font['weight'], `${path}.weight`, SELECT_FONT_WEIGHT_ALLOWED);
  validateStaticStringOrDynamicValue(attribute, font['family'], `${path}.family`, false);
  validateEnumStringValue(attribute, font['style'], `${path}.style`, SELECT_FONT_STYLE_ALLOWED);
}

function validateSelectStylesSchema(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined
): void {
  if (value === undefined || value === null) {
    return;
  }
  if (isDynamicValueShape(value)) {
    return;
  }
  if (!isRecordValue(value)) {
    recordSelectSchemaWarning(
      attribute,
      SchemaErrorCode.TYPE_MISMATCH,
      'styles',
      'Property styles expects object, private styles have been ignored'
    );
    return;
  }
  let styles: Record<string, Object> = value as Record<string, Object>;
  validateSelectFontSchema(attribute, styles['font'], 'styles.font');
  validateStaticColorOrDynamicValue(attribute, styles['fontColor'], 'styles.fontColor');
  validateStaticColorOrDynamicValue(attribute, styles['selectedOptionBgColor'], 'styles.selectedOptionBgColor');
  validateSelectFontSchema(attribute, styles['selectedOptionFont'], 'styles.selectedOptionFont');
  validateStaticColorOrDynamicValue(attribute, styles['selectedOptionFontColor'], 'styles.selectedOptionFontColor');
  validateStaticColorOrDynamicValue(attribute, styles['optionBgColor'], 'styles.optionBgColor');
  validateSelectFontSchema(attribute, styles['optionFont'], 'styles.optionFont');
  validateStaticColorOrDynamicValue(attribute, styles['optionFontColor'], 'styles.optionFontColor');
  validatePositiveNumberValue(attribute, styles['space'], 'styles.space', false);
  validateEnumStringValue(attribute, styles['arrowPosition'], 'styles.arrowPosition', ARROW_POSITION_ALLOWED);
  validateMenuAlignSchema(attribute, styles['menuAlign'], 'styles.menuAlign');
  validatePositiveNumberValue(attribute, styles['optionWidth'], 'styles.optionWidth');
  validatePositiveNumberValue(attribute, styles['optionHeight'], 'styles.optionHeight');
  validateStaticColorOrDynamicValue(attribute, styles['menuBackgroundColor'], 'styles.menuBackgroundColor');
  validateDividerSchema(attribute, styles['divider'], 'styles.divider');
}

function validateMenuAlignSchema(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined,
  path: string
): void {
  if (value === undefined || value === null || isDynamicObjectValueShape(value)) {
    return;
  }
  if (typeof value === 'string') {
    validateEnumStringValue(attribute, value as Object, path, MENU_ALIGN_ALLOWED);
    return;
  }
  if (!isRecordValue(value)) {
    recordSelectSchemaWarning(
      attribute,
      SchemaErrorCode.TYPE_MISMATCH,
      path,
      `Property ${path} expects string or object, fallback/reset has been applied`
    );
    return;
  }
  let align: Record<string, Object> = value as Record<string, Object>;
  let menuAlignKeys: string[] = ['alignType', 'offset'];
  reportSelectUnknownObjectMembers(attribute, path, align, menuAlignKeys);
  if (!hasKnownObjectMember(align, menuAlignKeys)) {
    return;
  }
  validateEnumStringValue(attribute, align['alignType'], `${path}.alignType`, MENU_ALIGN_ALLOWED);
  let offset: Object | undefined = align['offset'];
  if (offset !== undefined && offset !== null) {
    if (isDynamicObjectValueShape(offset)) {
      return;
    }
    if (!isRecordValue(offset)) {
      recordSelectSchemaWarning(
        attribute,
        SchemaErrorCode.TYPE_MISMATCH,
        `${path}.offset`,
        `Property ${path}.offset expects object, fallback/reset has been applied`
      );
      return;
    }
    let offsetRecord: Record<string, Object> = offset as Record<string, Object>;
    let offsetKeys: string[] = ['dx', 'dy'];
    reportSelectUnknownObjectMembers(attribute, `${path}.offset`, offsetRecord, offsetKeys);
    if (!hasKnownObjectMember(offsetRecord, offsetKeys)) {
      return;
    }
    validateFiniteNumberValue(attribute, offsetRecord['dx'], `${path}.offset.dx`);
    validateFiniteNumberValue(attribute, offsetRecord['dy'], `${path}.offset.dy`);
  }
}

function validateDividerSchema(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined,
  path: string
): void {
  if (value === undefined || value === null) {
    return;
  }
  if (isDynamicObjectValueShape(value)) {
    return;
  }
  if (!isRecordValue(value)) {
    recordSelectSchemaWarning(
      attribute,
      SchemaErrorCode.TYPE_MISMATCH,
      path,
      `Property ${path} expects object or null, fallback/reset has been applied`
    );
    return;
  }
  let divider: Record<string, Object> = value as Record<string, Object>;
  let dividerKeys: string[] = ['strokeWidth', 'color', 'startMargin', 'endMargin'];
  reportSelectUnknownObjectMembers(attribute, path, divider, dividerKeys);
  if (!hasKnownObjectMember(divider, dividerKeys)) {
    return;
  }
  validatePositiveNumberValue(attribute, divider['strokeWidth'], `${path}.strokeWidth`);
  validateStaticColorOrDynamicValue(attribute, divider['color'], `${path}.color`);
  validatePositiveNumberValue(attribute, divider['startMargin'], `${path}.startMargin`, false);
  validatePositiveNumberValue(attribute, divider['endMargin'], `${path}.endMargin`, false);
}

function validateSelectFontDynamicDfx(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined,
  path: string,
  forceDynamic: boolean = false
): void {
  let shouldResolveDynamic: boolean = forceDynamic || isDynamicValueShape(value);
  let font: Record<string, Object> | undefined = shouldResolveDynamic ?
    resolveDynamicRecordForDfx(attribute, value, path, forceDynamic) :
    (isRecordValue(value) ? value as Record<string, Object> : undefined);
  if (font === undefined) {
    return;
  }
  let nextForceDynamic: boolean = shouldResolveDynamic;
  if (nextForceDynamic) {
    let fontKeys: string[] = ['size', 'weight', 'family', 'style'];
    reportSelectUnknownObjectMembers(attribute, path, font, fontKeys);
    if (!hasKnownObjectMember(font, fontKeys)) {
      return;
    }
  }
  validateDynamicNumberResult(attribute, font['size'], `${path}.size`, 0, true, nextForceDynamic);
  validateDynamicEnumStringResult(attribute, font['weight'], `${path}.weight`, SELECT_FONT_WEIGHT_ALLOWED,
    nextForceDynamic);
  validateDynamicStringResult(attribute, font['family'], `${path}.family`, nextForceDynamic);
  validateDynamicEnumStringResult(attribute, font['style'], `${path}.style`, SELECT_FONT_STYLE_ALLOWED,
    nextForceDynamic);
}

function validateMenuAlignDynamicDfx(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined,
  path: string,
  forceDynamic: boolean = false
): void {
  let shouldValidate: boolean = forceDynamic || isDynamicValueShape(value);
  if (!shouldValidate && !isRecordValue(value)) {
    return;
  }
  let resolved: A2UIValueType | undefined = shouldValidate ? resolveSelectDfxValue(value, attribute, forceDynamic) :
    value as A2UIValueType;
  if (resolved === undefined || resolved === null) {
    return;
  }
  if (typeof resolved === 'string') {
    if (shouldValidate && !MENU_ALIGN_ALLOWED.includes(resolved.toLowerCase())) {
      reportSelectDynamicInvalidValue(attribute, path, 'is out of enum range');
    }
    return;
  }
  if (!isRecordValue(resolved as Object | undefined)) {
    if (shouldValidate) {
      reportSelectDynamicTypeMismatch(attribute, path, 'string or object');
    }
    return;
  }
  let align: Record<string, Object> = resolved as Record<string, Object>;
  let nextForceDynamic: boolean = forceDynamic || isDynamicValueShape(value);
  if (nextForceDynamic) {
    let menuAlignKeys: string[] = ['alignType', 'offset'];
    reportSelectUnknownObjectMembers(attribute, path, align, menuAlignKeys);
    if (!hasKnownObjectMember(align, menuAlignKeys)) {
      return;
    }
  }
  validateDynamicEnumStringResult(attribute, align['alignType'], `${path}.alignType`, MENU_ALIGN_ALLOWED,
    nextForceDynamic);
  let offset: Object | undefined = align['offset'];
  let shouldResolveOffset: boolean = nextForceDynamic || isDynamicValueShape(offset);
  let offsetRecord: Record<string, Object> | undefined = shouldResolveOffset ?
    resolveDynamicRecordForDfx(attribute, offset, `${path}.offset`, nextForceDynamic) :
    (isRecordValue(offset) ? offset as Record<string, Object> : undefined);
  if (offsetRecord === undefined) {
    return;
  }
  let offsetKeys: string[] = ['dx', 'dy'];
  let offsetDynamic: boolean = nextForceDynamic || isDynamicValueShape(offset);
  if (offsetDynamic) {
    reportSelectUnknownObjectMembers(attribute, `${path}.offset`, offsetRecord, offsetKeys);
    if (!hasKnownObjectMember(offsetRecord, offsetKeys)) {
      return;
    }
  }
  validateDynamicNumberResult(attribute, offsetRecord['dx'], `${path}.offset.dx`, undefined, false,
    offsetDynamic);
  validateDynamicNumberResult(attribute, offsetRecord['dy'], `${path}.offset.dy`, undefined, false,
    offsetDynamic);
}

function validateDividerDynamicDfx(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined,
  path: string,
  forceDynamic: boolean = false
): void {
  if (value === undefined || value === null) {
    return;
  }
  let shouldResolveDynamic: boolean = forceDynamic || isDynamicValueShape(value);
  let divider: Record<string, Object> | undefined = shouldResolveDynamic ?
    resolveDynamicRecordForDfx(attribute, value, path, forceDynamic) :
    (isRecordValue(value) ? value as Record<string, Object> : undefined);
  if (divider === undefined) {
    return;
  }
  let nextForceDynamic: boolean = shouldResolveDynamic;
  if (nextForceDynamic) {
    let dividerKeys: string[] = ['strokeWidth', 'color', 'startMargin', 'endMargin'];
    reportSelectUnknownObjectMembers(attribute, path, divider, dividerKeys);
    if (!hasKnownObjectMember(divider, dividerKeys)) {
      return;
    }
  }
  validateDynamicNumberResult(attribute, divider['strokeWidth'], `${path}.strokeWidth`, 0, true, nextForceDynamic);
  validateDynamicColorResult(attribute, divider['color'], `${path}.color`, nextForceDynamic);
  validateDynamicNumberResult(attribute, divider['startMargin'], `${path}.startMargin`, 0, false, nextForceDynamic);
  validateDynamicNumberResult(attribute, divider['endMargin'], `${path}.endMargin`, 0, false, nextForceDynamic);
}

function validateSelectSymbolIconDynamicDfx(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined,
  path: string,
  forceDynamic: boolean = false
): void {
  let shouldResolveDynamic: boolean = forceDynamic || isDynamicValueShape(value);
  let symbolIcon: Record<string, Object> | undefined = shouldResolveDynamic ?
    resolveDynamicRecordForDfx(attribute, value, path, forceDynamic) :
    (isRecordValue(value) ? value as Record<string, Object> : undefined);
  if (symbolIcon === undefined) {
    return;
  }
  let nextForceDynamic: boolean = shouldResolveDynamic;
  validateDynamicSymbolIconSrcResult(attribute, symbolIcon['src'], `${path}.src`, nextForceDynamic);
  validateDynamicNumberResult(attribute, symbolIcon['fontSize'], `${path}.fontSize`, 0, true, nextForceDynamic);
  validateDynamicNumberResult(attribute, symbolIcon['fontWeight'], `${path}.fontWeight`, 0, true, nextForceDynamic);
  validateDynamicColorResult(attribute, symbolIcon['fontColor'], `${path}.fontColor`, nextForceDynamic);
  validateDynamicEnumStringResult(attribute, symbolIcon['renderingStrategy'], `${path}.renderingStrategy`,
    [SYMBOL_RENDERING_SINGLE, SYMBOL_RENDERING_MULTIPLE_COLOR, SYMBOL_RENDERING_MULTIPLE_OPACITY], nextForceDynamic);
  validateDynamicEnumStringResult(attribute, symbolIcon['effectStrategy'], `${path}.effectStrategy`,
    [SYMBOL_EFFECT_NONE, SYMBOL_EFFECT_SCALE, SYMBOL_EFFECT_HIERARCHICAL], nextForceDynamic);
}

function validateSelectOptionsDynamicDfx(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined,
  path: string
): void {
  let dynamicOptions: boolean = isDynamicValueShape(value);
  let options: Object[] | undefined = dynamicOptions ?
    validateDynamicArrayResult(attribute, value, path) :
    (Array.isArray(value) ? value as Object[] : undefined);
  if (options === undefined) {
    return;
  }
  for (let index: number = 0; index < options.length; index++) {
    let item: Object = options[index];
    if (!isRecordValue(item)) {
      continue;
    }
    let itemRecord: Record<string, Object> = item as Record<string, Object>;
    let itemPath: string = `${path}[${index}]`;
    let optionKeys: string[] = ['value', 'icon', 'symbolIcon'];
    reportSelectUnknownObjectMembers(attribute, itemPath, itemRecord, optionKeys);
    validateDynamicStringResult(attribute, itemRecord['value'], `${itemPath}.value`, dynamicOptions);
    validateDynamicStringResult(attribute, itemRecord['icon'], `${itemPath}.icon`, dynamicOptions);
    validateSelectSymbolIconDynamicDfx(attribute, itemRecord['symbolIcon'], `${itemPath}.symbolIcon`, dynamicOptions);
  }
}

function validateSelectStylesDynamicDfx(
  attribute: CustomComponentAttribute | undefined,
  value: Object | undefined,
  path: string
): void {
  let dynamicStyles: boolean = isDynamicValueShape(value);
  let styles: Record<string, Object> | undefined = dynamicStyles ?
    resolveDynamicRecordForDfx(attribute, value, path) :
    (isRecordValue(value) ? value as Record<string, Object> : undefined);
  if (styles === undefined) {
    return;
  }
  validateSelectFontDynamicDfx(attribute, styles['font'], `${path}.font`, dynamicStyles);
  validateDynamicColorResult(attribute, styles['fontColor'], `${path}.fontColor`, dynamicStyles);
  validateDynamicColorResult(attribute, styles['selectedOptionBgColor'], `${path}.selectedOptionBgColor`,
    dynamicStyles);
  validateSelectFontDynamicDfx(attribute, styles['selectedOptionFont'], `${path}.selectedOptionFont`, dynamicStyles);
  validateDynamicColorResult(attribute, styles['selectedOptionFontColor'], `${path}.selectedOptionFontColor`,
    dynamicStyles);
  validateDynamicColorResult(attribute, styles['optionBgColor'], `${path}.optionBgColor`, dynamicStyles);
  validateSelectFontDynamicDfx(attribute, styles['optionFont'], `${path}.optionFont`, dynamicStyles);
  validateDynamicColorResult(attribute, styles['optionFontColor'], `${path}.optionFontColor`, dynamicStyles);
  validateDynamicNumberResult(attribute, styles['space'], `${path}.space`, 0, false, dynamicStyles);
  validateDynamicEnumStringResult(attribute, styles['arrowPosition'], `${path}.arrowPosition`,
    ARROW_POSITION_ALLOWED, dynamicStyles);
  validateMenuAlignDynamicDfx(attribute, styles['menuAlign'], `${path}.menuAlign`, dynamicStyles);
  validateDynamicNumberResult(attribute, styles['optionWidth'], `${path}.optionWidth`, 0, true, dynamicStyles);
  validateDynamicNumberResult(attribute, styles['optionHeight'], `${path}.optionHeight`, 0, true, dynamicStyles);
  validateDynamicColorResult(attribute, styles['menuBackgroundColor'], `${path}.menuBackgroundColor`, dynamicStyles);
  validateDividerDynamicDfx(attribute, styles['divider'], `${path}.divider`, dynamicStyles);
}

function validateExtendedSelectDynamicDfxForWarning(
  attribute: CustomComponentAttribute | undefined,
  parsed: Record<string, Object>
): void {
  validateSelectOptionsDynamicDfx(attribute, parsed['options'], 'options');
  validateDynamicNumberResult(attribute, parsed['selected'], 'selected', -1, false);
  validateDynamicStringResult(attribute, parsed['value'], 'value');
  validateSelectStylesDynamicDfx(attribute, parsed['styles'], 'styles');
}

export function validateExtendedSelectSchemaForWarning(attribute: CustomComponentAttribute | undefined): void {
  withAttributeSchemaWarningContext(attribute, (): void => {
    let parsed: Record<string, Object> | undefined = parseCustomProps(attribute, 'ExtendedSelect');
    if (parsed === undefined) {
      recordSelectSchemaWarning(
        attribute,
        SchemaErrorCode.REQUIRED_MISS,
        'options',
        'Property options is required, fallback to empty options'
      );
      recordSelectSchemaWarning(
        attribute,
        SchemaErrorCode.REQUIRED_MISS,
        'value',
        'Property value is required, fallback to empty string'
      );
      return;
    }
    let options: Object[] | undefined = parseSelectOptionsForSchema(attribute, parsed['options']);
    validateSelectOptionsArraySchema(attribute, options);
    if (parsed['selected'] !== undefined) {
      validateSelectedValue(attribute, parsed['selected']);
    }
    validateStaticStringOrDynamicValue(attribute, parsed['value'], 'value', true);
    validateSelectStylesSchema(attribute, parsed['styles']);
    validateExtendedSelectDynamicDfxForWarning(attribute, parsed);
  });
}

@Component
export struct ExtendedSelect {
  @Prop @Watch('onAttributeChange') attribute: CustomComponentAttribute | undefined = undefined;
  @State currentOptions: SelectOptionItem[] = [];
  @State currentSelected: number = -1;
  @State currentValue: string = '';
  @State currentFont: SelectFont = {};
  @State currentFontColor: string = '';
  @State currentSelectedOptionBgColor: string = '';
  @State currentSelectedOptionFont: SelectFont = {};
  @State currentSelectedOptionFontColor: string = '';
  @State currentOptionBgColor: string = '';
  @State currentOptionFont: SelectFont = {};
  @State currentOptionFontColor: string = '';
  @State currentSpace: number = 0;
  @State currentArrowPosition: string = DEFAULT_ARROW_POSITION;
  @State currentMenuAlign: MenuAlignConfig = createDefaultMenuAlign();
  @State currentOptionWidth: number = 0;
  @State currentOptionHeight: number = 0;
  @State currentMenuBackgroundColor: string = '';
  @State currentDivider: SelectDividerOptions = {};
  @State currentDividerSet: boolean = false;
  @State currentDividerNull: boolean = false;

  aboutToAppear(): void {
    this.applyOptions();
  }

  onAttributeChange(): void {
    this.applyOptions();
  }

  private applyOptions(): void {
    let options: ExtendedSelectOptions = this.resolveOptions();
    this.currentOptions = options.options;
    this.currentSelected = options.selected;
    this.currentValue = options.value;
    this.currentFont = options.font;
    this.currentFontColor = options.fontColor;
    this.currentSelectedOptionBgColor = options.selectedOptionBgColor;
    this.currentSelectedOptionFont = options.selectedOptionFont;
    this.currentSelectedOptionFontColor = options.selectedOptionFontColor;
    this.currentOptionBgColor = options.optionBgColor;
    this.currentOptionFont = options.optionFont;
    this.currentOptionFontColor = options.optionFontColor;
    this.currentSpace = options.space;
    this.currentArrowPosition = options.arrowPosition;
    this.currentMenuAlign = options.menuAlign;
    this.currentOptionWidth = options.optionWidth;
    this.currentOptionHeight = options.optionHeight;
    this.currentMenuBackgroundColor = options.menuBackgroundColor;
    this.currentDivider = options.dividerSet && !options.dividerNull ?
      (options.divider !== null ? options.divider as SelectDividerOptions : {}) : {};
    this.currentDividerSet = options.dividerSet;
    this.currentDividerNull = options.dividerNull;
  }

  build() {
    Column() {
      Select(this.buildSelectOptions())
        .selected(this.currentSelected)
        .value(this.resolveDisplayValue())
        .font(this.resolveFont(this.currentFont))
        .fontColor(this.currentFontColor.length > 0 ?
          this.currentFontColor : this.resolveThemeColor(SELECT_FONT_COLOR, SELECT_FONT_COLOR_DARK))
        .selectedOptionBgColor(this.currentSelectedOptionBgColor.length > 0 ?
          this.currentSelectedOptionBgColor : this.resolveThemeColor(SELECT_SELECTED_OPTION_BG_COLOR, SELECT_SELECTED_OPTION_BG_COLOR_DARK))
        .selectedOptionFont(this.resolveFont(this.currentSelectedOptionFont))
        .selectedOptionFontColor(this.currentSelectedOptionFontColor.length > 0 ?
          this.currentSelectedOptionFontColor : this.resolveThemeColor(SELECT_SELECTED_OPTION_FONT_COLOR, SELECT_SELECTED_OPTION_FONT_COLOR_DARK))
        .optionBgColor(this.currentOptionBgColor.length > 0 ?
          this.currentOptionBgColor : Color.Transparent)
        .optionFont(this.resolveFont(this.currentOptionFont))
        .optionFontColor(this.currentOptionFontColor.length > 0 ?
          this.currentOptionFontColor : this.resolveThemeColor(SELECT_OPTION_FONT_COLOR, SELECT_OPTION_FONT_COLOR_DARK))
        .space(this.currentSpace > 8 ? this.currentSpace : undefined)
        .arrowPosition(this.resolveArrowPosition())
        .menuAlign(this.resolveMenuAlignType(), this.resolveMenuAlignOffset())
        .optionWidth(this.currentOptionWidth > 0 ? this.currentOptionWidth : undefined)
        .optionHeight(this.currentOptionHeight > 0 ? this.currentOptionHeight : undefined)
        .menuBackgroundColor(this.currentMenuBackgroundColor.length > 0 ?
          this.currentMenuBackgroundColor : Color.Transparent)
        .menuBackgroundBlurStyle(this.currentMenuBackgroundColor.length > 0 ?
          BlurStyle.NONE : BlurStyle.COMPONENT_ULTRA_THICK)
        .divider(this.currentDividerNull ? null :
          (this.currentDividerSet ? this.resolveDivider() : undefined))
        .onSelect((index: number, text?: string) => {
          this.currentSelected = index;
          if (text) {
            this.currentValue = text;
          }
          this.dispatchSelectChange(index, text);
        })
        .onClick((event: ClickEvent) => {
          this.dispatchSelectClick(event);
        })
        .attributeModifier(buildExtendedCommonStyleModifier<SelectAttribute>(this.attribute, {
          backgroundColor: this.resolveThemeColor(SELECT_BACKGROUND_COLOR, SELECT_BACKGROUND_COLOR_DARK)
        }))
        .id(this.resolveComponentId())
        .layoutWeight(this.resolveWeight())
        .onAppear(() => {
          dispatchComponentEvent(this.attribute, 'onAppear');
        })
        .accessibilityText(this.resolveAccessibilityLabel())
        .accessibilityDescription(this.resolveAccessibilityDescription())
    }
  }

  private resolveDisplayValue(): string {
    if (this.currentValue.length > 0) {
      return this.currentValue;
    }
    if (this.currentSelected >= 0 && this.currentSelected < this.currentOptions.length) {
      return this.currentOptions[this.currentSelected].value;
    }
    return '';
  }

  private resolveIconResource(iconStr: string): ResourceStr {
    if (iconStr.startsWith('http://') || iconStr.startsWith('https://') ||
      iconStr.startsWith('resources/')) {
      return iconStr;
    }
    let resourceName: string = iconStr;
    let dotIdx: number = iconStr.lastIndexOf('.');
    if (dotIdx > 0) {
      resourceName = iconStr.substring(0, dotIdx);
    }
    return $r('app.media.' + resourceName);
  }

  private buildSelectOptions(): SelectOption[] {
    let result: SelectOption[] = [];
    for (let i = 0; i < this.currentOptions.length; i++) {
      let item: SelectOptionItem = this.currentOptions[i];
      let option: SelectOption = { value: item.value };
      if (item.icon !== undefined && item.icon.length > 0) {
        option.icon = this.resolveIconResource(item.icon);
      }
      if (item.symbolIcon !== undefined) {
        option.symbolIcon = this.resolveSymbolIconModifier(item.symbolIcon);
      }
      result.push(option);
    }
    return result;
  }

  private resolveSymbolIconModifier(symbolIcon: SelectSymbolIcon): SymbolGlyphModifier {
    let modifier: SymbolGlyphModifier = new SymbolGlyphModifier($r(symbolIcon.src));
    if (symbolIcon.fontSize !== undefined) {
      modifier.fontSize(symbolIcon.fontSize);
    }
    if (symbolIcon.fontWeight !== undefined) {
      modifier.fontWeight(symbolIcon.fontWeight);
    }
    if (symbolIcon.fontColor !== undefined && symbolIcon.fontColor.length > 0) {
      modifier.fontColor([symbolIcon.fontColor]);
    }
    let renderingStrategy: SymbolRenderingStrategy | undefined =
      this.resolveSymbolRenderingStrategy(symbolIcon.renderingStrategy);
    if (renderingStrategy !== undefined) {
      modifier.renderingStrategy(renderingStrategy);
    }
    let effectStrategy: SymbolEffectStrategy | undefined =
      this.resolveSymbolEffectStrategy(symbolIcon.effectStrategy);
    if (effectStrategy !== undefined) {
      modifier.effectStrategy(effectStrategy);
    }
    return modifier;
  }

  private resolveSymbolRenderingStrategy(renderingStrategy: string | undefined): SymbolRenderingStrategy | undefined {
    if (renderingStrategy === SYMBOL_RENDERING_SINGLE) {
      return SymbolRenderingStrategy.SINGLE;
    }
    if (renderingStrategy === SYMBOL_RENDERING_MULTIPLE_COLOR) {
      return SymbolRenderingStrategy.MULTIPLE_COLOR;
    }
    if (renderingStrategy === SYMBOL_RENDERING_MULTIPLE_OPACITY) {
      return SymbolRenderingStrategy.MULTIPLE_OPACITY;
    }
    return undefined;
  }

  private resolveSymbolEffectStrategy(effectStrategy: string | undefined): SymbolEffectStrategy | undefined {
    if (effectStrategy === SYMBOL_EFFECT_NONE) {
      return SymbolEffectStrategy.NONE;
    }
    if (effectStrategy === SYMBOL_EFFECT_SCALE) {
      return SymbolEffectStrategy.SCALE;
    }
    if (effectStrategy === SYMBOL_EFFECT_HIERARCHICAL) {
      return SymbolEffectStrategy.HIERARCHICAL;
    }
    return undefined;
  }

  private resolveComponentId(): string {
    return this.attribute?.id ?? '';
  }

  private resolveWeight(): number | string {
    let styleOptions: ExtendedCommonStyleOptions = resolveExtendedCommonStyleOptions(this.attribute);
    if (styleOptions.layoutWeight !== undefined) {
      return styleOptions.layoutWeight;
    }
    let weight = this.attribute?.properties?.weight;
    if (weight !== undefined && weight > 0) {
      return weight;
    }
    return 0;
  }

  private dispatchSelectChange(index: number, text?: string): void {
    let payload: Record<string, Object> = {
      'index': index as Object,
      'value': (text ?? '') as Object
    };
    dispatchComponentEvent(this.attribute, 'onSelect', payload);
    dispatchComponentEvent(this.attribute, 'onChange', payload);
  }

  private dispatchSelectClick(event?: ClickEvent): void {
    let payload: Record<string, Object> = {
      'x': (event?.x ?? 0) as Object,
      'y': (event?.y ?? 0) as Object
    };
    dispatchComponentEvent(this.attribute, 'onClick', payload);
  }

  private resolveThemeColor(lightColor: string, darkColor: string): string {
    return this.attribute?.componentTheme?.colorMode === ThemeMode.DARK ? darkColor : lightColor;
  }

  private resolveAccessibilityLabel(): string {
    return this.attribute?.properties?.accessibilityLabel ?? '';
  }

  private resolveAccessibilityDescription(): string {
    return this.attribute?.properties?.accessibilityDescription ?? '';
  }

  private resolveFont(font: SelectFont): Font {
    if (font.size === undefined && font.weight === undefined &&
      font.family === undefined && font.style === undefined) {
      let result: Font = {};
      return result;
    }
    let result: Font = {};
    if (font.size !== undefined) {
      result.size = font.size;
    }
    if (font.size !== undefined || font.weight !== undefined) {
      result.weight = this.resolveFontWeight(font.weight);
    }
    if (font.family !== undefined && font.family.length > 0) {
      result.family = font.family;
    }
    if (font.style !== undefined && font.style.length > 0) {
      result.style = this.resolveFontStyle(font.style);
    }
    return result;
  }

  private resolveFontWeight(weight: string | undefined): FontWeight {
    if (weight === undefined) {
      return FontWeight.Medium;
    }
    if (weight === 'Lighter') {
      return FontWeight.Lighter;
    }
    if (weight === 'Regular') {
      return FontWeight.Regular;
    }
    if (weight === 'Medium') {
      return FontWeight.Medium;
    }
    if (weight === 'Bold') {
      return FontWeight.Bold;
    }
    if (weight === 'Bolder') {
      return FontWeight.Bolder;
    }
    return FontWeight.Medium;
  }

  private resolveFontStyle(style: string): FontStyle {
    return style === 'Italic' || style === 'italic' ? FontStyle.Italic : FontStyle.Normal;
  }

  private resolveArrowPosition(): ArrowPosition {
    return this.currentArrowPosition === ARROW_POSITION_START
      ? ArrowPosition.START
      : ArrowPosition.END;
  }

  private resolveMenuAlignType(): MenuAlignType {
    if (this.currentMenuAlign.alignType === MENU_ALIGN_CENTER) {
      return MenuAlignType.CENTER;
    }
    if (this.currentMenuAlign.alignType === MENU_ALIGN_END) {
      return MenuAlignType.END;
    }
    return MenuAlignType.START;
  }

  private resolveMenuAlignOffset(): Offset | undefined {
    if (this.currentMenuAlign.offset === undefined) {
      return undefined;
    }
    let offset: Offset = {
      dx: this.currentMenuAlign.offset.dx,
      dy: this.currentMenuAlign.offset.dy
    };
    return offset;
  }

  private resolveDivider(): DividerOptions {
    let div: SelectDividerOptions = this.currentDivider;
    if (div.strokeWidth === undefined && div.color === undefined &&
      div.startMargin === undefined && div.endMargin === undefined) {
      let result: DividerOptions = {};
      return result;
    }
    let result: DividerOptions = {};
    if (div.strokeWidth !== undefined) {
      result.strokeWidth = div.strokeWidth;
    }
    if (div.color !== undefined) {
      result.color = div.color;
    }
    if (div.startMargin !== undefined) {
      result.startMargin = div.startMargin;
    }
    if (div.endMargin !== undefined) {
      result.endMargin = div.endMargin;
    }
    return result;
  }

  private resolveOptions(): ExtendedSelectOptions {
    validateExtendedSelectSchemaForWarning(this.attribute);
    let parsed = parseCustomProps(this.attribute, 'ExtendedSelect');
    if (parsed === undefined) {
      return createDefaultSelectOptions();
    }

    let options: ExtendedSelectOptions = createDefaultSelectOptions();
    let styles: Record<string, Object> | undefined = this.resolveStyleProps(parsed);

    this.resolveOptionsArray(parsed, options);

    let selectedValue: Object | undefined = parsed['selected'];
    if (selectedValue !== undefined && selectedValue !== null) {
      let num: number | undefined = resolveSelectNumberValue(selectedValue, this.attribute);
      if (num !== undefined) {
        options.selected = num;
      }
    }

    let valueText: string | undefined = resolveDynamicStringOptionValue(parsed['value'], this.attribute);
    if (valueText !== undefined) {
      options.value = valueText;
    }

    this.resolveFontOption(this.resolveStyleValue(styles, 'font'), options.font);
    this.resolveColorOption(this.resolveStyleValue(styles, 'fontColor'), (c: string) => {
      options.fontColor = c;
    });
    this.resolveColorOption(
      this.resolveStyleValue(styles, 'selectedOptionBgColor'),
      (c: string) => { options.selectedOptionBgColor = c; }
    );
    this.resolveFontOption(this.resolveStyleValue(styles, 'selectedOptionFont'), options.selectedOptionFont);
    this.resolveColorOption(
      this.resolveStyleValue(styles, 'selectedOptionFontColor'),
      (c: string) => { options.selectedOptionFontColor = c; }
    );
    this.resolveColorOption(this.resolveStyleValue(styles, 'optionBgColor'), (c: string) => {
      options.optionBgColor = c;
    });
    this.resolveFontOption(this.resolveStyleValue(styles, 'optionFont'), options.optionFont);
    this.resolveColorOption(this.resolveStyleValue(styles, 'optionFontColor'), (c: string) => {
      options.optionFontColor = c;
    });

    let spaceValue: Object | undefined = this.resolveStyleValue(styles, 'space');
    if (spaceValue !== undefined && spaceValue !== null) {
      let num: number | undefined = resolveSelectNumberValue(spaceValue, this.attribute);
      if (num !== undefined && num >= 0) {
        options.space = num;
      }
    }

    let arrowValue: string | undefined =
      resolveDynamicStringOptionValue(this.resolveStyleValue(styles, 'arrowPosition'), this.attribute);
    let normalizedArrowValue: string | undefined = this.normalizeEnumValue(arrowValue, ARROW_POSITION_ALLOWED);
    if (normalizedArrowValue !== undefined) {
      options.arrowPosition = normalizedArrowValue;
    }

    this.resolveMenuAlignOption(this.resolveStyleValue(styles, 'menuAlign'), options.menuAlign);

    let optionWidthValue: Object | undefined = this.resolveStyleValue(styles, 'optionWidth');
    if (optionWidthValue !== undefined && optionWidthValue !== null) {
      let num: number | undefined = resolveSelectNumberValue(optionWidthValue, this.attribute);
      if (num !== undefined && num > 0) {
        options.optionWidth = num;
      }
    }

    let optionHeightValue: Object | undefined = this.resolveStyleValue(styles, 'optionHeight');
    if (optionHeightValue !== undefined && optionHeightValue !== null) {
      let num: number | undefined = resolveSelectNumberValue(optionHeightValue, this.attribute);
      if (num !== undefined && num > 0) {
        options.optionHeight = num;
      }
    }

    this.resolveColorOption(this.resolveStyleValue(styles, 'menuBackgroundColor'), (c: string) => {
      options.menuBackgroundColor = c;
    });

    let dividerValue: Object | undefined = this.resolveStyleValue(styles, 'divider');
    if (dividerValue !== undefined && dividerValue !== null && typeof dividerValue === 'object') {
      this.resolveDividerOption(dividerValue, options);
    } else if (dividerValue === null) {
      options.dividerSet = true;
      options.dividerNull = true;
      options.divider = null;
    }

    return options;
  }

  private resolveStyleProps(parsed: Record<string, Object>): Record<string, Object> | undefined {
    return resolveSelectRecordValue(parsed['styles'], this.attribute);
  }

  private resolveStyleValue(styles: Record<string, Object> | undefined, key: string): Object | undefined {
    if (styles === undefined) {
      return undefined;
    }
    return resolveSelectDynamicValue(styles[key], this.attribute) as Object | undefined;
  }

  private resolveOptionsArray(source: Record<string, Object>, options: ExtendedSelectOptions): void {
    let raw: Object[] | undefined = resolveSelectArrayValue(source['options'], this.attribute);
    if (raw === undefined) {
      raw = parseJsonArrayOptionValue(source['options'], 'ExtendedSelect', 'options');
    }
    if (raw === undefined) {
      return;
    }

    let items: SelectOptionItem[] = [];
    for (let i = 0; i < raw.length; i++) {
      let item: Object = raw[i];
      if (item === null || typeof item !== 'object' || Array.isArray(item)) {
        continue;
      }
      let map: Record<string, Object> = item as Record<string, Object>;
      let val: string | undefined = resolveDynamicStringOptionValue(map['value'], this.attribute);
      if (val === undefined || val.length === 0) {
        continue;
      }
      let icon: string | undefined = resolveDynamicStringOptionValue(map['icon'], this.attribute);
      let symbolIcon: SelectSymbolIcon | undefined = this.resolveSymbolIconOption(map['symbolIcon']);
      let optionItem: SelectOptionItem = { value: val };
      if (icon !== undefined && icon.length > 0) {
        optionItem.icon = icon;
      }
      if (symbolIcon !== undefined) {
        optionItem.symbolIcon = symbolIcon;
      }
      items.push(optionItem);
    }
    options.options = items;
  }

  private resolveSymbolIconOption(value: Object | undefined): SelectSymbolIcon | undefined {
    let resolvedValue: Record<string, Object> | undefined = resolveSelectRecordValue(value, this.attribute);
    if (resolvedValue === undefined) {
      return undefined;
    }
    let map: Record<string, Object> = resolvedValue;
    let src: string | undefined = resolveDynamicStringOptionValue(map['src'], this.attribute);
    if (src === undefined || src.length === 0) {
      return undefined;
    }
    let resourceName: string | undefined = resolveIconSystemSymbolResourceName(src);
    if (resourceName === undefined) {
      return undefined;
    }

    let symbolIcon: SelectSymbolIcon = { src: resourceName };
    let fontSize: number | undefined = this.resolvePositiveNumber(map['fontSize']);
    if (fontSize !== undefined) {
      symbolIcon.fontSize = fontSize;
    }
    let fontWeight: number | undefined = this.resolvePositiveNumber(map['fontWeight']);
    if (fontWeight !== undefined) {
      symbolIcon.fontWeight = fontWeight;
    }
    let fontColor: string | undefined = resolveDynamicStringOptionValue(map['fontColor'], this.attribute);
    if (fontColor !== undefined && fontColor.length > 0) {
      symbolIcon.fontColor = fontColor;
    }
    let renderingStrategy: string | undefined =
      resolveDynamicStringOptionValue(map['renderingStrategy'], this.attribute);
    if (this.resolveSymbolRenderingStrategy(renderingStrategy) !== undefined) {
      symbolIcon.renderingStrategy = renderingStrategy;
    }
    let effectStrategy: string | undefined = resolveDynamicStringOptionValue(map['effectStrategy'], this.attribute);
    if (this.resolveSymbolEffectStrategy(effectStrategy) !== undefined) {
      symbolIcon.effectStrategy = effectStrategy;
    }
    return symbolIcon;
  }

  private resolvePositiveNumber(value: Object | undefined): number | undefined {
    let num: number | undefined = resolveSelectNumberValue(value, this.attribute);
    if (num === undefined || num <= 0) {
      return undefined;
    }
    return num;
  }

  private resolveColorOption(value: Object | undefined, setter: (c: string) => void): void {
    if (value === undefined) {
      return;
    }
    let colorStr: string | undefined = resolveDynamicStringOptionValue(value, this.attribute);
    if (colorStr === undefined || !isValidSelectColorString(colorStr)) {
      setter('');
      return;
    }
    setter(colorStr);
  }

  private resolveFontOption(value: Object | undefined, target: SelectFont): void {
    let map: Record<string, Object> | undefined = resolveSelectRecordValue(value, this.attribute);
    if (map === undefined) {
      return;
    }

    let sizeValue: Object | undefined = map['size'];
    if (sizeValue !== undefined && sizeValue !== null) {
      let num: number | undefined = resolveSelectNumberValue(sizeValue, this.attribute);
      if (num !== undefined && num > 0) {
        target.size = num;
      }
    }

    let weightValue: string | undefined = resolveDynamicStringOptionValue(map['weight'], this.attribute);
    if (weightValue !== undefined) {
      target.weight = weightValue;
    }

    let familyValue: string | undefined = resolveDynamicStringOptionValue(map['family'], this.attribute);
    if (familyValue !== undefined) {
      target.family = familyValue;
    }

    let styleValue: string | undefined = resolveDynamicStringOptionValue(map['style'], this.attribute);
    if (styleValue !== undefined) {
      target.style = styleValue;
    }
  }

  private resolveMenuAlignOption(value: Object | undefined, target: MenuAlignConfig): void {
    if (value === undefined || value === null) {
      return;
    }
    if (typeof value === 'string') {
      let alignStr: string | undefined = this.normalizeEnumValue(value, MENU_ALIGN_ALLOWED);
      if (alignStr !== undefined) {
        target.alignType = alignStr;
      }
      return;
    }
    if (typeof value === 'object' && !Array.isArray(value)) {
      let map: Record<string, Object> = value as Record<string, Object>;
      let alignStr: string | undefined = resolveDynamicStringOptionValue(map['alignType'], this.attribute);
      let normalizedAlignStr: string | undefined = this.normalizeEnumValue(alignStr, MENU_ALIGN_ALLOWED);
      if (normalizedAlignStr !== undefined) {
        target.alignType = normalizedAlignStr;
      }
      let offsetValue: Object | undefined = map['offset'];
      if (offsetValue !== undefined && offsetValue !== null) {
        let offsetMap: Record<string, Object> | undefined = resolveSelectRecordValue(offsetValue, this.attribute);
        if (offsetMap === undefined) {
          return;
        }
        let dxValue: Object | undefined = offsetMap['dx'];
        let dyValue: Object | undefined = offsetMap['dy'];
        let dx: number | undefined = dxValue !== undefined ? resolveSelectNumberValue(dxValue, this.attribute) : 0;
        let dy: number | undefined = dyValue !== undefined ? resolveSelectNumberValue(dyValue, this.attribute) : 0;
        if (dx !== undefined && dy !== undefined) {
          let offset: MenuAlignOffset = { dx: dx, dy: dy };
          target.offset = offset;
        }
      }
    }
  }

  private normalizeEnumValue(value: string | undefined, allowedValues: string[]): string | undefined {
    if (value === undefined) {
      return undefined;
    }
    let normalizedValue: string = value.toLowerCase();
    return allowedValues.includes(normalizedValue) ? normalizedValue : undefined;
  }

  private resolveDividerOption(value: Object, options: ExtendedSelectOptions): void {
    options.dividerSet = true;
    let map: Record<string, Object> | undefined = resolveSelectRecordValue(value, this.attribute);
    if (map === undefined) {
      options.dividerSet = false;
      return;
    }
    let div: SelectDividerOptions = {};

    let strokeWidthValue: Object | undefined = map['strokeWidth'];
    if (strokeWidthValue !== undefined && strokeWidthValue !== null) {
      let num: number | undefined = resolveSelectNumberValue(strokeWidthValue, this.attribute);
      if (num !== undefined && num > 0) {
        div.strokeWidth = num;
      }
    }

    let colorValue: string | undefined = resolveDynamicStringOptionValue(map['color'], this.attribute);
    if (colorValue !== undefined) {
      div.color = colorValue;
    }

    let startMarginValue: Object | undefined = map['startMargin'];
    if (startMarginValue !== undefined && startMarginValue !== null) {
      let num: number | undefined = resolveSelectNumberValue(startMarginValue, this.attribute);
      if (num !== undefined && num >= 0) {
        div.startMargin = num;
      }
    }

    let endMarginValue: Object | undefined = map['endMargin'];
    if (endMarginValue !== undefined && endMarginValue !== null) {
      let num: number | undefined = resolveSelectNumberValue(endMarginValue, this.attribute);
      if (num !== undefined && num >= 0) {
        div.endMargin = num;
      }
    }

    options.divider = div;
  }
}

@Builder
export function extendedSelectBuilder(attribute: CustomComponentAttribute): void {
  ExtendedSelect({ attribute: attribute });
}

export function createExtendedSelectDefinition(): CustomComponentDefinition {
  const schemaProvider: SchemaProvider = (_version: string): string => {
    return SchemaResourceLoader.loadSchema('schema/Extended/components/ExtendedSelect.json');
  };

  return {
    type: EXTENDED_SELECT_TYPE,
    builder: wrapBuilder(extendedSelectBuilder),
    schemaProvider: schemaProvider
  };
}
