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

import { CustomComponentAttribute } from '../A2UI/CustomComponent';
import { parseCustomProps } from '../A2UI/CustomComponentUtils';
import { FunctionBridge } from '../../functions/FunctionBridge';
import { A2UIDynamicValue } from '../../types/DynamicValue';
import { A2UIFunctionCall } from '../../types/FunctionCall';
import { A2UIValueType } from '../../types/types';
import { A2UIResolveContext, ResolveResult } from '../../types/runtime/ResolveContext';
import { DynamicValueResolver } from '../../types/runtime/DynamicValueResolver';
import { TypeGuards } from '../../types/runtime/TypeGuards';
import { SchemaErrorCode, SchemaErrorInfoManager } from '../../schema/SchemaErrorInfoManager';

type ExtendedLengthValue = Length | LayoutPolicy;
type ExtendedBackgroundImageSizeValue = SizeOptions | ImageSize;
type ExtendedShadowValue = ShadowOptions | ShadowStyle;

export interface ExtendedCommonStyleOptions {
  width?: ExtendedLengthValue;
  height?: ExtendedLengthValue;
  constraintSize?: ConstraintSizeOptions;
  backgroundImage?: ResourceStr;
  backgroundImageSize?: ExtendedBackgroundImageSizeValue;
  margin?: Length | Padding;
  borderRadius?: Length | BorderRadiuses;
  visibility?: Visibility;
  clip?: boolean;
  backgroundColor?: ResourceColor;
  borderWidth?: Length | EdgeWidths;
  borderColor?: ResourceColor | EdgeColors;
  padding?: Length | Padding;
  layoutWeight?: number | string;
  flexShrink?: number;
  aspectRatio?: number;
  shadow?: ExtendedShadowValue;
  linearGradient?: LinearGradientOptions;
}

export interface ExtendedCommonStyleState {
  options?: ExtendedCommonStyleOptions;
}

const COMMON_STYLE_TAG: string = 'ExtendedCommonStyleModifier';
const BACKGROUND_IMAGE_SIZE_KEYS: string[] = ['backgroundImageSizeWithStyle', 'backgroundImageSize', 'backgroundimageSize'];
const LENGTH_PATTERN: RegExp = /^-?\d+(\.\d+)?(fp|vp|%)?$/;
const COLOR_PATTERN: RegExp = /^#([0-9a-fA-F]{6}|[0-9a-fA-F]{8})$/;

export class ExtendedCommonStyleModifier<T extends CommonMethod<T>> implements AttributeModifier<T> {
  private readonly options: ExtendedCommonStyleOptions;

  constructor(
    attribute: CustomComponentAttribute | undefined,
    defaults: ExtendedCommonStyleOptions = {},
    state?: ExtendedCommonStyleState
  ) {
    let result = resolveExtendedCommonStyleOptionsWithStatus(attribute, defaults.backgroundColor);
    this.options = mergeExtendedCommonStyleOptions(
      defaults,
      result.options,
      state?.options,
      result.failedDynamicProperties
    );
    if (state !== undefined) {
      state.options = this.options;
    }
  }

  applyNormalAttribute(instance: T): void {
    applyExtendedCommonStyleOptions(instance, this.options);
  }
}

export function buildExtendedCommonStyleModifier<T extends CommonMethod<T>>(
  attribute: CustomComponentAttribute | undefined,
  defaults: ExtendedCommonStyleOptions = {},
  state?: ExtendedCommonStyleState
): AttributeModifier<T> {
  return new ExtendedCommonStyleModifier<T>(attribute, defaults, state);
}

export function mergeExtendedCommonStyleOptions(
  defaults: ExtendedCommonStyleOptions,
  resolved: ExtendedCommonStyleOptions,
  previous?: ExtendedCommonStyleOptions,
  failedDynamicProperties: string[] = []
): ExtendedCommonStyleOptions {
  let merged: ExtendedCommonStyleOptions = {
    width: resolved.width ?? defaults.width,
    height: resolved.height ?? defaults.height,
    constraintSize: resolved.constraintSize ?? defaults.constraintSize,
    backgroundImage: resolved.backgroundImage ?? defaults.backgroundImage,
    backgroundImageSize: resolved.backgroundImageSize ?? defaults.backgroundImageSize,
    margin: resolved.margin ?? defaults.margin,
    borderRadius: resolved.borderRadius ?? defaults.borderRadius,
    visibility: resolved.visibility ?? defaults.visibility,
    clip: resolved.clip ?? defaults.clip,
    backgroundColor: resolved.backgroundColor ?? defaults.backgroundColor,
    borderWidth: resolved.borderWidth ?? defaults.borderWidth,
    borderColor: resolved.borderColor ?? defaults.borderColor,
    padding: resolved.padding ?? defaults.padding,
    layoutWeight: resolved.layoutWeight ?? defaults.layoutWeight,
    flexShrink: resolved.flexShrink ?? defaults.flexShrink,
    aspectRatio: resolved.aspectRatio ?? defaults.aspectRatio,
    shadow: resolved.shadow ?? defaults.shadow,
    linearGradient: resolved.linearGradient ?? defaults.linearGradient
  };
  if (previous === undefined) {
    return merged;
  }

  if (failedDynamicProperties.indexOf('width') >= 0 && previous.width !== undefined) {
    merged.width = previous.width;
  }
  if (failedDynamicProperties.indexOf('height') >= 0 && previous.height !== undefined) {
    merged.height = previous.height;
  }
  if (failedDynamicProperties.indexOf('constraintSize') >= 0 && previous.constraintSize !== undefined) {
    merged.constraintSize = previous.constraintSize;
  }
  if (failedDynamicProperties.indexOf('backgroundImage') >= 0 && previous.backgroundImage !== undefined) {
    merged.backgroundImage = previous.backgroundImage;
  }
  if (failedDynamicProperties.indexOf('backgroundImageSize') >= 0 && previous.backgroundImageSize !== undefined) {
    merged.backgroundImageSize = previous.backgroundImageSize;
  }
  if (failedDynamicProperties.indexOf('margin') >= 0 && previous.margin !== undefined) {
    merged.margin = previous.margin;
  }
  if (failedDynamicProperties.indexOf('borderRadius') >= 0 && previous.borderRadius !== undefined) {
    merged.borderRadius = previous.borderRadius;
  }
  if (failedDynamicProperties.indexOf('visibility') >= 0 && previous.visibility !== undefined) {
    merged.visibility = previous.visibility;
  }
  if (failedDynamicProperties.indexOf('clip') >= 0 && previous.clip !== undefined) {
    merged.clip = previous.clip;
  }
  if (failedDynamicProperties.indexOf('backgroundColor') >= 0 && previous.backgroundColor !== undefined) {
    merged.backgroundColor = previous.backgroundColor;
  }
  if (failedDynamicProperties.indexOf('borderWidth') >= 0 && previous.borderWidth !== undefined) {
    merged.borderWidth = previous.borderWidth;
  }
  if (failedDynamicProperties.indexOf('borderColor') >= 0 && previous.borderColor !== undefined) {
    merged.borderColor = previous.borderColor;
  }
  if (failedDynamicProperties.indexOf('padding') >= 0 && previous.padding !== undefined) {
    merged.padding = previous.padding;
  }
  if (failedDynamicProperties.indexOf('layoutWeight') >= 0 && previous.layoutWeight !== undefined) {
    merged.layoutWeight = previous.layoutWeight;
  }
  if (failedDynamicProperties.indexOf('flexShrink') >= 0 && previous.flexShrink !== undefined) {
    merged.flexShrink = previous.flexShrink;
  }
  if (failedDynamicProperties.indexOf('aspectRatio') >= 0 && previous.aspectRatio !== undefined) {
    merged.aspectRatio = previous.aspectRatio;
  }
  if (failedDynamicProperties.indexOf('shadow') >= 0 && previous.shadow !== undefined) {
    merged.shadow = previous.shadow;
  }
  if (failedDynamicProperties.indexOf('linearGradient') >= 0 && previous.linearGradient !== undefined) {
    merged.linearGradient = previous.linearGradient;
  }
  return merged;
}

interface ExtendedCommonStyleResolveResult {
  options: ExtendedCommonStyleOptions;
  failedDynamicProperties: string[];
}

function resolveExtendedCommonStyleOptionsWithStatus(
  attribute: CustomComponentAttribute | undefined,
  backgroundColorFallback: ResourceColor = 'transparent'
): ExtendedCommonStyleResolveResult {
  let customProps = parseCustomProps(attribute, COMMON_STYLE_TAG);
  let styles = resolveStyles(customProps) ?? {};

  let resolver = new ExtendedCommonStyleResolver(attribute, styles, backgroundColorFallback);
  return {
    options: resolver.resolve(),
    failedDynamicProperties: resolver.getFailedDynamicProperties()
  };
}

export function resolveExtendedCommonStyleOptions(
  attribute: CustomComponentAttribute | undefined,
  backgroundColorFallback: ResourceColor = 'transparent'
): ExtendedCommonStyleOptions {
  return resolveExtendedCommonStyleOptionsWithStatus(attribute, backgroundColorFallback).options;
}

export function applyExtendedCommonStyleOptions<T extends CommonMethod<T>>(
  instance: T,
  options: ExtendedCommonStyleOptions
): void {
  if (options.width !== undefined) {
    instance.width(options.width);
  }
  if (options.height !== undefined) {
    instance.height(options.height);
  }
  if (options.constraintSize !== undefined) {
    instance.constraintSize(options.constraintSize);
  }
  if (options.backgroundImage !== undefined) {
    instance.backgroundImage(options.backgroundImage);
  }
  if (options.backgroundImageSize !== undefined) {
    instance.backgroundImageSize(options.backgroundImageSize);
  }
  if (options.margin !== undefined) {
    instance.margin(options.margin);
  }
  if (options.borderRadius !== undefined) {
    instance.borderRadius(options.borderRadius);
  }
  if (options.visibility !== undefined) {
    instance.visibility(options.visibility);
  }
  if (options.clip !== undefined) {
    instance.clip(options.clip);
  }
  if (options.backgroundColor !== undefined) {
    instance.backgroundColor(options.backgroundColor);
  }
  if (options.borderWidth !== undefined) {
    instance.borderWidth(options.borderWidth);
  }
  if (options.borderColor !== undefined) {
    instance.borderColor(options.borderColor);
  }
  if (options.padding !== undefined) {
    instance.padding(options.padding);
  }
  if (options.layoutWeight !== undefined) {
    instance.layoutWeight(options.layoutWeight);
  }
  if (options.flexShrink !== undefined) {
    instance.flexShrink(options.flexShrink);
  }
  if (options.aspectRatio !== undefined) {
    instance.aspectRatio(options.aspectRatio);
  }
  if (options.shadow !== undefined) {
    instance.shadow(options.shadow);
  }
  if (options.linearGradient !== undefined) {
    instance.linearGradient(options.linearGradient);
  }
}

function resolveStyles(customProps: Record<string, Object> | undefined): Record<string, Object> | undefined {
  if (customProps === undefined) {
    return undefined;
  }
  let styleValue = customProps['styles'];
  if (typeof styleValue === 'object' && styleValue !== null && !Array.isArray(styleValue)) {
    return styleValue as Record<string, Object>;
  }
  return undefined;
}

function isRecord(value: Object | undefined): boolean {
  return typeof value === 'object' && value !== null && !Array.isArray(value);
}

function hasOwnProperty(record: Record<string, Object>, key: string): boolean {
  return Object.keys(record).indexOf(key) >= 0;
}

function isNonNegativeNumber(value: number): boolean {
  return Number.isFinite(value) && value >= 0;
}

class ExtendedCommonStyleResolver {
  private readonly attribute: CustomComponentAttribute | undefined;
  private readonly styles: Record<string, Object>;
  private readonly backgroundColorFallback: ResourceColor;
  private readonly failedDynamicProperties: string[] = [];

  constructor(
    attribute: CustomComponentAttribute | undefined,
    styles: Record<string, Object>,
    backgroundColorFallback: ResourceColor
  ) {
    this.attribute = attribute;
    this.styles = styles;
    this.backgroundColorFallback = backgroundColorFallback;
  }

  getFailedDynamicProperties(): string[] {
    return this.failedDynamicProperties.slice();
  }

  resolve(): ExtendedCommonStyleOptions {
    let options: ExtendedCommonStyleOptions = {};

    this.resolveWidth(options);
    this.resolveHeight(options);
    this.resolveConstraintSize(options);
    this.resolveBackgroundImage(options);
    this.resolveBackgroundImageSize(options);
    this.resolveMargin(options);
    this.resolveBorderRadius(options);
    this.resolveVisibility(options);
    this.resolveClip(options);
    this.resolveBackgroundColor(options);
    this.resolveBorderWidth(options);
    this.resolveBorderColor(options);
    this.resolvePadding(options);
    this.resolveLayoutWeight(options);
    this.resolveFlexShrink(options);
    this.resolveAspectRatio(options);
    this.resolveShadow(options);
    this.resolveLinearGradient(options);
    this.normalizeZeroConstraintBounds(options);

    return options;
  }

  private resolveWidth(options: ExtendedCommonStyleOptions): void {
    let raw = this.readStyle('width');
    let resolved = this.resolveLengthOrPolicy(raw);
    if (resolved !== undefined) {
      options.width = resolved;
    }
  }

  private resolveHeight(options: ExtendedCommonStyleOptions): void {
    let raw = this.readStyle('height');
    let resolved = this.resolveLengthOrPolicy(raw);
    if (resolved !== undefined) {
      options.height = resolved;
    }
  }

  private resolveConstraintSize(options: ExtendedCommonStyleOptions): void {
    let raw = this.readStyle('constraintSize');
    let resolved = this.resolveEdgeLengthObject(raw, ['minWidth', 'maxWidth', 'minHeight', 'maxHeight']);
    if (resolved !== undefined) {
      options.constraintSize = resolved as ConstraintSizeOptions;
    }
  }

  private resolveBackgroundImage(options: ExtendedCommonStyleOptions): void {
    let raw = this.readStyle('backgroundImage');
    let value = this.resolveString(raw);
    if (value !== undefined && value.length > 0) {
      options.backgroundImage = value;
    }
  }

  private resolveBackgroundImageSize(options: ExtendedCommonStyleOptions): void {
    let raw: Object | undefined = undefined;
    let hasConfiguredSize: boolean = false;
    for (const key of BACKGROUND_IMAGE_SIZE_KEYS) {
      if (!hasOwnProperty(this.styles, key)) {
        continue;
      }
      hasConfiguredSize = true;
      raw = this.readStyle(key, 'backgroundImageSize');
      if (raw !== undefined) {
        break;
      }
    }

    let resolved = this.resolveBackgroundImageSizeValue(raw);
    if (resolved !== undefined) {
      options.backgroundImageSize = resolved;
    } else if (hasConfiguredSize) {
      options.backgroundImageSize = ImageSize.Auto;
    }
  }

  private resolveMargin(options: ExtendedCommonStyleOptions): void {
    let raw = this.readStyle('margin');
    let resolved = this.resolveEdgeLengthStyle(raw, ['top', 'right', 'bottom', 'left']);
    if (resolved !== undefined) {
      options.margin = resolved as Length | Padding;
    }
  }

  private resolveBorderRadius(options: ExtendedCommonStyleOptions): void {
    let raw = this.readStyle('borderRadius');
    let resolved = this.resolveEdgeLengthStyle(raw, ['topLeft', 'topRight', 'bottomLeft', 'bottomRight']);
    if (resolved !== undefined) {
      options.borderRadius = resolved as Length | BorderRadiuses;
    }
  }

  private resolveVisibility(options: ExtendedCommonStyleOptions): void {
    let raw = this.readStyle('visibility');
    if (raw === undefined) {
      return;
    }
    options.visibility = this.resolveVisibilityValue(raw) ?? Visibility.Visible;
  }

  private resolveClip(options: ExtendedCommonStyleOptions): void {
    let raw = this.readStyle('clip');
    if (raw === undefined) {
      return;
    }
    options.clip = this.resolveBoolean(raw) ?? false;
  }

  private resolveBackgroundColor(options: ExtendedCommonStyleOptions): void {
    let raw = this.readStyle('backgroundColor');
    if (raw === undefined) {
      return;
    }
    options.backgroundColor = this.resolveColor(raw) ?? this.backgroundColorFallback;
  }

  private resolveBorderWidth(options: ExtendedCommonStyleOptions): void {
    let raw = this.readStyle('borderWidth');
    if (raw === undefined) {
      return;
    }
    let resolved = this.resolveEdgeLengthStyle(raw, ['top', 'right', 'bottom', 'left']);
    options.borderWidth = resolved !== undefined ? resolved as Length | EdgeWidths : 0;
  }

  private resolveBorderColor(options: ExtendedCommonStyleOptions): void {
    let raw = this.readStyle('borderColor');
    if (raw === undefined) {
      return;
    }
    let resolved = this.resolveEdgeColorStyle(raw);
    options.borderColor = resolved !== undefined ? resolved : Color.Black;
  }

  private resolvePadding(options: ExtendedCommonStyleOptions): void {
    let raw = this.readStyle('padding');
    let resolved = this.resolveEdgeLengthStyle(raw, ['top', 'right', 'bottom', 'left']);
    if (resolved !== undefined) {
      options.padding = resolved as Length | Padding;
    }
  }

  private resolveLayoutWeight(options: ExtendedCommonStyleOptions): void {
    let raw = this.readStyle('layoutWeight');
    if (raw === undefined) {
      return;
    }
    let resolved = this.resolveNumberOrNumericString(raw);
    if (resolved === undefined) {
      options.layoutWeight = 1;
      return;
    }
    let numeric = typeof resolved === 'number' ? resolved : Number(resolved);
    options.layoutWeight = Number.isFinite(numeric) && numeric >= 0 ? resolved : 1;
  }

  private resolveFlexShrink(options: ExtendedCommonStyleOptions): void {
    let hasConfiguredValue = hasOwnProperty(this.styles, 'flexShrink');
    let raw = this.readStyle('flexShrink');
    if (raw === undefined) {
      if (hasConfiguredValue || this.attribute?.properties?.resetFlexShrinkToParentDefault === true) {
        let parentDefault = this.resolveParentFlexShrinkDefault();
        if (parentDefault !== undefined) {
          options.flexShrink = parentDefault;
        }
      }
      return;
    }
    let value = this.resolveNumber(raw);
    if (value === undefined || value < 0) {
      options.flexShrink = this.resolveParentFlexShrinkDefault() ?? 0;
      return;
    }
    options.flexShrink = value;
  }

  private resolveParentFlexShrinkDefault(): number | undefined {
    let value = this.attribute?.properties?.flexShrinkParentDefault;
    return value === 0 || value === 1 ? value : undefined;
  }

  private resolveAspectRatio(options: ExtendedCommonStyleOptions): void {
    let raw = this.readStyle('aspectRatio');
    if (raw === undefined) {
      return;
    }
    let componentType = this.attribute?.type ?? '';
    let value = this.resolveNumber(raw);
    if (value !== undefined && value > 0) {
      options.aspectRatio = value;
    } else if (value !== undefined) {
      SchemaErrorInfoManager.getInstance().recordSchemaWarning(
        SchemaErrorCode.INVALID_VALUE,
        `Property aspectRatio got invalid number value, fallback to default value`,
        'styles.aspectRatio',
        'component',
        componentType
      );
      options.aspectRatio = 1;
    } else {
      let typeName = typeof raw === 'object' && raw === null ? 'null' : typeof raw;
      SchemaErrorInfoManager.getInstance().recordSchemaWarning(
        SchemaErrorCode.TYPE_MISMATCH,
        `Property aspectRatio expects number value, got type '${typeName}', fallback to default value`,
        'styles.aspectRatio',
        'component',
        componentType
      );
      options.aspectRatio = 1;
    }
  }

  private resolveShadow(options: ExtendedCommonStyleOptions): void {
    let raw = this.readStyle('shadow');
    if (raw === undefined) {
      return;
    }
    let resolved = this.resolveShadowValue(raw);
    if (resolved !== undefined) {
      options.shadow = resolved;
    }
  }

  private resolveLinearGradient(options: ExtendedCommonStyleOptions): void {
    let raw = this.readStyle('linearGradient');
    let resolved = this.resolveLinearGradientValue(raw);
    if (resolved !== undefined) {
      options.linearGradient = resolved;
    }
  }

  private normalizeZeroConstraintBounds(options: ExtendedCommonStyleOptions): void {
    let constraintSize = options.constraintSize as Record<string, Object> | undefined;
    if (constraintSize === undefined) {
      return;
    }
    let hasZeroMaxWidth = this.isZeroLengthValue(constraintSize['maxWidth']);
    let hasZeroMaxHeight = this.isZeroLengthValue(constraintSize['maxHeight']);
    if (hasZeroMaxWidth) {
      options.width = 0;
    }
    if (hasZeroMaxHeight) {
      options.height = 0;
    }
    if (hasZeroMaxWidth && hasZeroMaxHeight) {
      options.padding = 0;
      options.borderWidth = 0;
    }
  }

  private readStyle(key: string, optionKey: string = key): Object | undefined {
    if (!hasOwnProperty(this.styles, key)) {
      return undefined;
    }
    let raw = this.styles[key];
    let resolved = this.resolveDynamicValue(raw);
    if (resolved === undefined && this.isDynamicStyleValue(raw) &&
      this.failedDynamicProperties.indexOf(optionKey) < 0) {
      this.failedDynamicProperties.push(optionKey);
    }
    return resolved;
  }

  private isDynamicStyleValue(value: Object | undefined): boolean {
    if (typeof value === 'string') {
      let trimmed = value.trim();
      return trimmed.startsWith('{{') && trimmed.endsWith('}}');
    }
    return TypeGuards.isDataBinding(value) || TypeGuards.isFunctionCall(value);
  }

  private resolveDynamicValue(value: Object | undefined): Object | undefined {
    if (value === undefined) {
      return undefined;
    }
    if (this.attribute?.resolver !== undefined) {
      return this.attribute.resolver.evaluateValue<Object | undefined>(value as A2UIValueType);
    }
    let resolveContext = this.buildResolveContext();
    let result = DynamicValueResolver.resolveValue(value as A2UIDynamicValue, resolveContext);
    if (!result.success) {
      return undefined;
    }
    return result.value as Object | undefined;
  }

  private buildResolveContext(): A2UIResolveContext | undefined {
    if (this.attribute === undefined) {
      return undefined;
    }
    let surfaceId: string = this.attribute.surfaceId ?? '';
    let componentId: string = this.attribute.id ?? '';
    let renderId: number = this.attribute.renderId ?? -1;
    return {
      renderId: renderId,
      surfaceId: surfaceId,
      componentId: componentId,
      dataModel: this.attribute.dataModel,
      invokeFunction: (functionCall: A2UIFunctionCall): A2UIValueType => {
        return FunctionBridge.invoke(functionCall, surfaceId, componentId, renderId);
      }
    } as A2UIResolveContext;
  }

  private resolveLengthOrPolicy(value: Object | undefined): ExtendedLengthValue | undefined {
    let layoutPolicy = this.resolveLayoutPolicy(value);
    if (layoutPolicy !== undefined) {
      return layoutPolicy;
    }
    return this.resolveLength(value);
  }

  private resolveLayoutPolicy(value: Object | undefined): LayoutPolicy | undefined {
    if (typeof value !== 'string') {
      return undefined;
    }
    let normalized = value.trim();
    if (normalized.length === 0) {
      return undefined;
    }
    let lower = normalized.toLowerCase();
    if (lower === 'matchparent' || lower === 'match_parent') {
      return LayoutPolicy.matchParent;
    }
    if (lower === 'wrapcontent' || lower === 'wrap_content') {
      return LayoutPolicy.wrapContent;
    }
    if (lower === 'fixatidealsize' || lower === 'fix_at_ideal_size') {
      return LayoutPolicy.fixAtIdealSize;
    }
    return undefined;
  }

  private resolveLength(value: Object | undefined): Length | undefined {
    if (typeof value === 'number' && isNonNegativeNumber(value)) {
      return value;
    }
    if (typeof value !== 'string') {
      return undefined;
    }
    let normalized = value.trim();
    if (normalized.length === 0) {
      return undefined;
    }
    if (!LENGTH_PATTERN.test(normalized)) {
      return undefined;
    }
    let numericPart = normalized.replace(/(fp|vp|%)$/, '');
    let parsed = Number(numericPart);
    if (!isNonNegativeNumber(parsed)) {
      return undefined;
    }
    return normalized;
  }

  private isZeroLengthValue(value: Object | undefined): boolean {
    if (typeof value === 'number') {
      return Number.isFinite(value) && value === 0;
    }
    if (typeof value !== 'string') {
      return false;
    }
    let normalized = value.trim();
    if (normalized.length === 0 || !LENGTH_PATTERN.test(normalized)) {
      return false;
    }
    let numericPart = normalized.replace(/(fp|vp|%)$/, '');
    return Number(numericPart) === 0;
  }

  private resolveNumber(value: Object | undefined): number | undefined {
    if (typeof value === 'number' && Number.isFinite(value)) {
      return value;
    }
    if (typeof value !== 'string') {
      return undefined;
    }
    let parsed = Number(value.trim());
    if (Number.isFinite(parsed)) {
      return parsed;
    }
    return undefined;
  }

  private resolveNumberOrNumericString(value: Object | undefined): number | string | undefined {
    if (typeof value === 'number' && Number.isFinite(value)) {
      return value;
    }
    if (typeof value !== 'string') {
      return undefined;
    }
    let normalized = value.trim();
    if (normalized.length === 0 || Number.isNaN(Number(normalized))) {
      return undefined;
    }
    return normalized;
  }

  private resolveString(value: Object | undefined): string | undefined {
    if (typeof value === 'string') {
      return value;
    }
    return undefined;
  }

  private resolveBoolean(value: Object | undefined): boolean | undefined {
    if (typeof value === 'boolean') {
      return value;
    }
    if (typeof value === 'string') {
      let normalized = value.trim().toLowerCase();
      if (normalized === 'true') {
        return true;
      }
      if (normalized === 'false') {
        return false;
      }
    }
    return undefined;
  }

  private resolveColor(value: Object | undefined): ResourceColor | undefined {
    if (typeof value !== 'string') {
      return undefined;
    }
    let normalized = value.trim();
    if (normalized === 'transparent' || COLOR_PATTERN.test(normalized)) {
      return normalized;
    }
    return undefined;
  }

  private resolveEdgeLengthStyle(value: Object | undefined, keys: string[]): Length | Padding | BorderRadiuses | undefined {
    let length = this.resolveLength(value);
    if (length !== undefined) {
      return length;
    }
    let resolved = this.resolveEdgeLengthObject(value, keys);
    return resolved as Padding | BorderRadiuses | undefined;
  }

  private resolveEdgeLengthObject(value: Object | undefined, keys: string[]): Padding | BorderRadiuses | ConstraintSizeOptions | undefined {
    if (!isRecord(value)) {
      return undefined;
    }
    let record = value as Record<string, Object>;
    let result: Record<string, Length> = {};
    for (const key of keys) {
      let raw = hasOwnProperty(record, key) ? this.resolveDynamicValue(record[key]) : undefined;
      let length = this.resolveLength(raw);
      if (length !== undefined) {
        result[key] = length;
      }
    }
    if (Object.keys(result).length === 0) {
      return undefined;
    }
    return result as Padding | BorderRadiuses | ConstraintSizeOptions;
  }

  private resolveEdgeColorStyle(value: Object | undefined): ResourceColor | EdgeColors | undefined {
    let color = this.resolveColor(value);
    if (color !== undefined) {
      return color;
    }
    if (!isRecord(value)) {
      return undefined;
    }

    let record = value as Record<string, Object>;
    let keys: string[] = ['top', 'right', 'bottom', 'left'];
    let result: Record<string, ResourceColor> = {};
    for (const key of keys) {
      let raw = hasOwnProperty(record, key) ? this.resolveDynamicValue(record[key]) : undefined;
      let resolved = this.resolveColor(raw);
      if (resolved !== undefined) {
        result[key] = resolved;
      }
    }
    if (Object.keys(result).length === 0) {
      return undefined;
    }
    return result as EdgeColors;
  }

  private resolveBackgroundImageSizeValue(value: Object | undefined): ExtendedBackgroundImageSizeValue | undefined {
    if (typeof value === 'string') {
      let normalized = value.trim().toLowerCase();
      if (normalized === 'cover') {
        return ImageSize.Cover;
      }
      if (normalized === 'contain') {
        return ImageSize.Contain;
      }
      if (normalized === 'auto') {
        return ImageSize.Auto;
      }
      if (normalized === 'fill') {
        return ImageSize.FILL;
      }
      return ImageSize.Auto;
    }
    if (!isRecord(value)) {
      return undefined;
    }
    let record = value as Record<string, Object>;
    let hasWidth = hasOwnProperty(record, 'width');
    let hasHeight = hasOwnProperty(record, 'height');
    if (!hasWidth && !hasHeight) {
      return undefined;
    }
    let width = hasWidth ? this.resolveLength(this.resolveDynamicValue(record['width'])) : undefined;
    let height = hasHeight ? this.resolveLength(this.resolveDynamicValue(record['height'])) : undefined;
    // An absent or empty-string dimension maps to LENGTH 0, matching NODE_BACKGROUND_IMAGE_SIZE
    // where ParseJsDimensionVp("") / a missing property resolve to value=0/unit=LENGTH. The rendering
    // layer then derives that dimension from the other one by the source-image aspect ratio, instead
    // of falling back to COVER/CONTAIN/AUTO. See image_painter.cpp CalculateBgImageSize.
    let sizeOptions: SizeOptions = {
      width: width !== undefined ? width : 0,
      height: height !== undefined ? height : 0
    };
    return sizeOptions;
  }

  private resolveVisibilityValue(value: Object | undefined): Visibility | undefined {
    if (typeof value !== 'string') {
      return undefined;
    }
    let normalized = value.trim().toLowerCase();
    if (normalized === 'visible') {
      return Visibility.Visible;
    }
    if (normalized === 'hidden') {
      return Visibility.Hidden;
    }
    if (normalized === 'none') {
      return Visibility.None;
    }
    return undefined;
  }

  private resolveShadowValue(value: Object | undefined): ExtendedShadowValue | undefined {
    let shadowStyle = this.resolveShadowStyle(value);
    if (shadowStyle !== undefined) {
      return shadowStyle;
    }
    if (!isRecord(value)) {
      return undefined;
    }

    let record = value as Record<string, Object>;
    let styleFromObject = hasOwnProperty(record, 'style') ? this.resolveShadowStyle(this.resolveDynamicValue(record['style'])) : undefined;
    if (styleFromObject !== undefined) {
      return styleFromObject;
    }

    let radius = hasOwnProperty(record, 'radius') ? this.resolveNumber(this.resolveDynamicValue(record['radius'])) : 0;
    let offsetX = hasOwnProperty(record, 'offsetX') ? this.resolveNumber(this.resolveDynamicValue(record['offsetX'])) : 0;
    let offsetY = hasOwnProperty(record, 'offsetY') ? this.resolveNumber(this.resolveDynamicValue(record['offsetY'])) : 0;
    let color = hasOwnProperty(record, 'color') ? this.resolveColor(this.resolveDynamicValue(record['color'])) : Color.Black;
    let fill = hasOwnProperty(record, 'fill') ? this.resolveBoolean(this.resolveDynamicValue(record['fill'])) : false;
    let type = hasOwnProperty(record, 'type') ? this.resolveShadowType(this.resolveDynamicValue(record['type'])) : ShadowType.COLOR;

    return {
      radius: radius !== undefined && radius > 0 ? radius : 0,
      offsetX: offsetX ?? 0,
      offsetY: offsetY ?? 0,
      color: color ?? Color.Black,
      fill: fill ?? false,
      type: type ?? ShadowType.COLOR
    } as ShadowOptions;
  }

  private resolveShadowStyle(value: Object | undefined): ShadowStyle | undefined {
    if (typeof value === 'number' && Number.isInteger(value) && value >= 0 && value <= 5) {
      return value as ShadowStyle;
    }
    if (typeof value !== 'string') {
      return undefined;
    }
    let normalized = value.trim();
    if (normalized.length === 0) {
      return undefined;
    }
    let lower = normalized.toLowerCase();
    if (lower === 'outerdefaultxs') {
      return ShadowStyle.OUTER_DEFAULT_XS;
    }
    if (lower === 'outerdefaultsm') {
      return ShadowStyle.OUTER_DEFAULT_SM;
    }
    if (lower === 'outerdefaultmd') {
      return ShadowStyle.OUTER_DEFAULT_MD;
    }
    if (lower === 'outerdefaultlg') {
      return ShadowStyle.OUTER_DEFAULT_LG;
    }
    if (lower === 'outerfloatingsm') {
      return ShadowStyle.OUTER_FLOATING_SM;
    }
    if (lower === 'outerfloatingmd') {
      return ShadowStyle.OUTER_FLOATING_MD;
    }
    let parsed = Number(normalized);
    if (Number.isInteger(parsed) && parsed >= 0 && parsed <= 5) {
      return parsed as ShadowStyle;
    }
    return undefined;
  }

  private resolveShadowType(value: Object | undefined): ShadowType | undefined {
    if (typeof value === 'number') {
      if (value === 0) {
        return ShadowType.COLOR;
      }
      if (value === 1) {
        return ShadowType.BLUR;
      }
    }
    if (typeof value !== 'string') {
      return undefined;
    }
    let normalized = value.trim().toLowerCase();
    if (normalized === 'color') {
      return ShadowType.COLOR;
    }
    if (normalized === 'blur') {
      return ShadowType.BLUR;
    }
    return undefined;
  }

  private resolveLinearGradientValue(value: Object | undefined): LinearGradientOptions | undefined {
    if (!isRecord(value)) {
      return undefined;
    }
    let record = value as Record<string, Object>;
    let colors = this.resolveGradientColors(record);
    if (colors.length === 0) {
      return undefined;
    }

    let angleValue = hasOwnProperty(record, 'angle') ? this.resolveDynamicValue(record['angle']) : undefined;
    let directionValue = hasOwnProperty(record, 'direction') ? this.resolveDynamicValue(record['direction']) : undefined;
    let repeatingValue = hasOwnProperty(record, 'repeating') ? this.resolveDynamicValue(record['repeating']) : undefined;

    return {
      angle: this.resolveGradientAngle(angleValue) ?? 180,
      direction: this.resolveGradientDirection(directionValue) ?? GradientDirection.Bottom,
      colors: colors,
      repeating: this.resolveBoolean(repeatingValue) ?? false
    } as LinearGradientOptions;
  }

  private resolveGradientAngle(value: Object | undefined): number | string | undefined {
    if (typeof value === 'number' && Number.isFinite(value)) {
      return value;
    }
    if (typeof value === 'string' && value.trim().length > 0) {
      return value.trim();
    }
    return undefined;
  }

  private resolveGradientDirection(value: Object | undefined): GradientDirection | undefined {
    if (typeof value === 'number' && Number.isInteger(value) && value >= 0 && value <= 8) {
      return value as GradientDirection;
    }
    if (typeof value !== 'string') {
      return undefined;
    }
    let normalized = value.trim().toLowerCase();
    if (normalized === 'left') {
      return GradientDirection.Left;
    }
    if (normalized === 'top') {
      return GradientDirection.Top;
    }
    if (normalized === 'right') {
      return GradientDirection.Right;
    }
    if (normalized === 'bottom') {
      return GradientDirection.Bottom;
    }
    if (normalized === 'lefttop') {
      return GradientDirection.LeftTop;
    }
    if (normalized === 'leftbottom') {
      return GradientDirection.LeftBottom;
    }
    if (normalized === 'righttop') {
      return GradientDirection.RightTop;
    }
    if (normalized === 'rightbottom') {
      return GradientDirection.RightBottom;
    }
    if (normalized === 'none') {
      return GradientDirection.None;
    }
    return undefined;
  }

  private resolveGradientColors(record: Record<string, Object>): Array<[ResourceColor, number]> {
    let colorsValue = hasOwnProperty(record, 'colors') ? this.resolveDynamicValue(record['colors']) : undefined;
    if (!Array.isArray(colorsValue)) {
      return [];
    }

    let stops = this.resolveGradientStops(record);
    let colors: [ResourceColor, number][] = [];
    let colorItems = colorsValue as Object[];
    for (let i = 0; i < colorItems.length; i++) {
      let colorItem = colorItems[i];
      let color: ResourceColor | undefined = undefined;
      let stop: number | undefined = undefined;
      if (Array.isArray(colorItem) && colorItem.length >= 2) {
        color = this.resolveColor(this.resolveDynamicValue(colorItem[0]));
        stop = this.resolveNumber(this.resolveDynamicValue(colorItem[1]));
      } else if (isRecord(colorItem)) {
        let colorRecord = colorItem as Record<string, Object>;
        let colorValue = hasOwnProperty(colorRecord, 'color') ?
          this.resolveDynamicValue(colorRecord['color']) : undefined;
        let stopValue = hasOwnProperty(colorRecord, 'stop') ?
          this.resolveDynamicValue(colorRecord['stop']) : undefined;
        color = this.resolveColor(colorValue);
        stop = this.resolveNumber(stopValue) ??
          (i < stops.length ? stops[i] : this.resolveDefaultStop(i, colorItems.length));
      } else {
        color = this.resolveColor(this.resolveDynamicValue(colorItem));
        stop = i < stops.length ? stops[i] : this.resolveDefaultStop(i, colorItems.length);
      }
      if (color !== undefined && stop !== undefined) {
        colors.push([color, Math.max(0, Math.min(1, stop))]);
      }
    }
    return colors;
  }

  private resolveGradientStops(record: Record<string, Object>): number[] {
    let stopsValue = hasOwnProperty(record, 'stops') ? this.resolveDynamicValue(record['stops']) : undefined;
    if (!Array.isArray(stopsValue)) {
      return [];
    }
    let result: number[] = [];
    let stops = stopsValue as Object[];
    for (const stop of stops) {
      let resolved = this.resolveNumber(this.resolveDynamicValue(stop));
      if (resolved !== undefined) {
        result.push(Math.max(0, Math.min(1, resolved)));
      }
    }
    return result;
  }

  private resolveDefaultStop(index: number, total: number): number {
    if (total <= 1) {
      return 0;
    }
    return index / (total - 1);
  }
}
