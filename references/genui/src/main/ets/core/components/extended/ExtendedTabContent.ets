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

import { NodeContent } from '@kit.ArkUI';
import { CustomComponentAttribute } from '../A2UI/CustomComponent';
import { CustomComponentDefinition } from '../A2UI/CustomComponentFactory';
import { A2UIValueType, SchemaProvider } from '../../types/types';
import { SchemaResourceLoader } from '../../schema/SchemaResourceLoader';
import {
  parseCustomProps,
  resolveDynamicStringOptionValue,
  resolveDynamicStringPathOptionValue,
  dispatchComponentEvent
} from '../A2UI/CustomComponentUtils';
import { ExtendedTabsRegistry, ExtendedTabMetadata, TabFontWeight } from './ExtendedTabsRegistry';
import { A2UIDynamicNumber } from '../../types/DynamicNumber';
import { A2UIDynamicValue } from '../../types/DynamicValue';
import { A2UIFunctionCall } from '../../types/FunctionCall';
import { A2UIResolveContext, ResolveResult } from '../../types/runtime/ResolveContext';
import { DynamicValueResolver } from '../../types/runtime/DynamicValueResolver';
import { FunctionBridge } from '../../functions/FunctionBridge';
import {
  buildExtendedCommonStyleModifier,
  ExtendedCommonStyleState
} from './ExtendedCommonStyleModifier';
import { ThemeMode } from '../../base/NativeEngineBridge';

const EXTENDED_TAB_CONTENT_TYPE: string = 'TabContent';
const DEFAULT_TAB_FONT_WEIGHT: TabFontWeight = 500;
const DEFAULT_TAB_CONTENT_TITLE: string = '';
const DEFAULT_TAB_CONTENT_ICON: string = '';
const DEFAULT_TAB_CONTENT_SELECTED_SRC: string = '';
const DEFAULT_TAB_CONTENT_TYPE_VALUE: string = '';
const DEFAULT_TAB_CONTENT_AREA_BACKGROUND_COLOR_LIGHT: string = '#FFFFFF';
const DEFAULT_TAB_CONTENT_AREA_BACKGROUND_COLOR_DARK: string = '#171E2B';
const A2UI_BINDINGS_KEY: string = '__a2uiBindings';
const TAB_CONTENT_HEX_COLOR_PATTERN: RegExp = /^#([0-9a-fA-F]{6}|[0-9a-fA-F]{8})$/;

export function isExtendedTabContentExpressionString(value: string): boolean {
  let trimmed = value.trim();
  return trimmed.startsWith('{{') && trimmed.endsWith('}}');
}

export function resolveExtendedTabContentRuntimeStringValue(
  value: Object | undefined,
  attribute: CustomComponentAttribute | undefined
): string | undefined {
  if (typeof value === 'string' && !isExtendedTabContentExpressionString(value)) {
    return value;
  }
  return resolveDynamicStringOptionValue(value, attribute);
}

export function normalizeExtendedTabTypeValue(value: string | undefined, fallback: string): string {
  if (value === undefined) {
    return fallback;
  }
  let normalized = value.trim().toLowerCase();
  if (normalized === 'capsule' || normalized === 'underline') {
    return normalized;
  }
  return fallback;
}

export function normalizeExtendedTabContentMetricValue(value: number): number | undefined {
  if (Number.isFinite(value) && value >= 0) {
    return value;
  }
  return undefined;
}

export function normalizeExtendedTabContentFontWeightValue(value: Object | undefined): TabFontWeight | undefined {
  if (typeof value === 'number' && Number.isFinite(value) && Number.isInteger(value) &&
    value >= 100 && value <= 900 && value % 100 === 0) {
    return value;
  }
  if (typeof value !== 'string') {
    return undefined;
  }
  let normalized = value.trim().toLowerCase();
  if (normalized === 'lighter' || normalized === 'normal' || normalized === 'regular' ||
    normalized === 'medium' || normalized === 'bold' || normalized === 'bolder') {
    return normalized;
  }
  return undefined;
}

export function normalizeExtendedTabContentColorValue(value: string | undefined): string | undefined {
  if (value === undefined || !TAB_CONTENT_HEX_COLOR_PATTERN.test(value)) {
    return undefined;
  }
  return value;
}

@Component
export struct ExtendedTabContent {
  private readonly tag: string = 'ExtendedTabContent';
  private commonStyleState: ExtendedCommonStyleState = {};
  @Prop @Watch('onAttributeChange') attribute: CustomComponentAttribute | undefined = undefined;

  aboutToAppear(): void {
    this.syncMetadata();
  }

  onAttributeChange(): void {
    this.syncMetadata();
  }

  aboutToDisappear(): void {
    ExtendedTabsRegistry.removeMetadata(this.attribute?.surfaceId ?? '', this.attribute?.id ?? '');
  }

  build() {
    Column() {
      if (this.attribute?.slot !== undefined) {
        ContentSlot(this.attribute?.slot as NodeContent)
      }
    }
    .onClick(() => {
      dispatchComponentEvent(this.attribute, 'onClick');
    })
    .onAppear(() => {
      dispatchComponentEvent(this.attribute, 'onAppear');
    })
    .accessibilityText(this.resolveAccessibilityLabel())
    .accessibilityDescription(this.resolveAccessibilityDescription())
    .id(this.attribute?.id ?? '')
    .attributeModifier(buildExtendedCommonStyleModifier<ColumnAttribute>(this.attribute, {
      width: '100%',
      height: '100%',
      layoutWeight: this.resolveWeight()
    }, this.commonStyleState))
  }

  private syncMetadata(): void {
    let componentId: string = this.attribute?.id ?? '';
    if (componentId.length === 0) {
      return;
    }
    let surfaceId: string = this.attribute?.surfaceId ?? '';
    let customProps = parseCustomProps(this.attribute, this.tag);
    let styleProps = this.resolveStyleProps(customProps);
    let metadata: ExtendedTabMetadata = {
      title: this.readDynamicString(customProps, 'title', DEFAULT_TAB_CONTENT_TITLE),
      icon: this.readDynamicString(customProps, 'icon', DEFAULT_TAB_CONTENT_ICON),
      selectedSrc: this.readDynamicString(customProps, 'selectedSrc', DEFAULT_TAB_CONTENT_SELECTED_SRC),
      tabType: this.readTabType(customProps, 'tabType', DEFAULT_TAB_CONTENT_TYPE_VALUE),
      selectedColor: this.readOptionalStyleColor(styleProps, 'selectedColor'),
      unSelectedColor: this.readOptionalStyleColor(styleProps, 'unSelectedColor'),
      defaultBackgroundColor: this.readOptionalStyleColor(styleProps, 'defaultBackgroundColor'),
      selectedBackgroundColor: this.readOptionalStyleColor(styleProps, 'selectedBackgroundColor'),
      defaultBorderColor: this.readOptionalStyleColor(styleProps, 'defaultBorderColor'),
      selectedBorderColor: this.readOptionalStyleColor(styleProps, 'selectedBorderColor'),
      fontSize: this.readOptionalStyleNumber(styleProps, 'fontSize'),
      fontWeight: this.readOptionalStyleFontWeight(styleProps, 'fontWeight') ?? DEFAULT_TAB_FONT_WEIGHT,
      iconSize: this.readOptionalStyleNumber(styleProps, 'iconSize'),
      space: this.readOptionalStyleNumber(styleProps, 'space')
    };
    ExtendedTabsRegistry.updateMetadata(surfaceId, componentId, metadata);
  }

  private readDynamicString(
    customProps: Record<string, Object> | undefined,
    key: string,
    fallback: string
  ): string {
    if (customProps === undefined) {
      return fallback;
    }
    let value = customProps[key];
    let resolved: string | undefined = this.resolveRuntimeStringValue(value);
    if (resolved !== undefined) {
      return resolved;
    }
    let bindingResolved: string | undefined = this.resolveBoundStringValue(customProps, key);
    if (bindingResolved !== undefined) {
      return bindingResolved;
    }
    return fallback;
  }

  private readTabType(
    customProps: Record<string, Object> | undefined,
    key: string,
    fallback: string
  ): string {
    if (customProps === undefined) {
      return fallback;
    }
    let resolved: string | undefined = this.resolveRuntimeStringValue(customProps[key]);
    if (resolved === undefined) {
      return fallback;
    }
    return normalizeExtendedTabTypeValue(resolved, fallback);
  }

  private readOptionalStyleString(
    styleProps: Record<string, Object> | undefined,
    key: string
  ): string | undefined {
    if (styleProps === undefined) {
      return undefined;
    }
    let value = styleProps[key];
    let resolved: string | undefined = this.resolveRuntimeStringValue(value);
    if (resolved !== undefined && resolved.length > 0) {
      return resolved;
    }
    return undefined;
  }

  private readOptionalStyleColor(
    styleProps: Record<string, Object> | undefined,
    key: string
  ): string | undefined {
    return normalizeExtendedTabContentColorValue(this.readOptionalStyleString(styleProps, key));
  }

  private resolveBoundStringValue(
    customProps: Record<string, Object> | undefined,
    key: string
  ): string | undefined {
    let bindingPath: string | undefined = this.resolveBindingPath(customProps, key);
    if (bindingPath === undefined) {
      return undefined;
    }
    return resolveDynamicStringPathOptionValue(bindingPath, this.attribute);
  }

  private resolveBindingPath(
    customProps: Record<string, Object> | undefined,
    key: string
  ): string | undefined {
    if (customProps === undefined) {
      return undefined;
    }
    let bindingsValue: Object | undefined = customProps[A2UI_BINDINGS_KEY];
    if (typeof bindingsValue !== 'object' || bindingsValue === null || Array.isArray(bindingsValue)) {
      return undefined;
    }
    let bindings = bindingsValue as Record<string, Object>;
    let bindingPath: Object | undefined = bindings[key];
    if (typeof bindingPath === 'string' && bindingPath.length > 0) {
      return bindingPath;
    }
    return undefined;
  }

  private resolveRuntimeStringValue(value: Object | undefined): string | undefined {
    return resolveExtendedTabContentRuntimeStringValue(value, this.attribute);
  }

  private readOptionalStyleNumber(
    styleProps: Record<string, Object> | undefined,
    key: string
  ): number | undefined {
    if (styleProps === undefined) {
      return undefined;
    }
    let value = styleProps[key];
    if (typeof value === 'number' && Number.isFinite(value)) {
      return normalizeExtendedTabContentMetricValue(value);
    }
    if (value !== undefined) {
      let resolveContext: A2UIResolveContext | undefined = this.buildResolveContext();
      let resolved: number = DynamicValueResolver.resolveNumber(value as A2UIDynamicNumber, Number.NaN, resolveContext);
      if (!Number.isNaN(resolved) && Number.isFinite(resolved)) {
        return normalizeExtendedTabContentMetricValue(resolved);
      }
    }
    return undefined;
  }

  private readOptionalStyleFontWeight(
    styleProps: Record<string, Object> | undefined,
    key: string
  ): TabFontWeight | undefined {
    if (styleProps === undefined) {
      return undefined;
    }
    let value = styleProps[key];
    let normalized = normalizeExtendedTabContentFontWeightValue(value);
    if (normalized !== undefined) {
      return normalized;
    }
    if (value !== undefined) {
      let resolveContext: A2UIResolveContext | undefined = this.buildResolveContext();
      let result = DynamicValueResolver.resolveValue(value as A2UIDynamicValue, resolveContext);
      if (result.success) {
        return normalizeExtendedTabContentFontWeightValue(result.value as Object | undefined);
      }
    }
    return undefined;
  }

  private resolveStyleProps(
    customProps: Record<string, Object> | undefined
  ): Record<string, Object> | undefined {
    if (customProps === undefined) {
      return undefined;
    }
    let styleValue = customProps['styles'];
    if (typeof styleValue === 'object' && styleValue !== null && !Array.isArray(styleValue)) {
      return styleValue as Record<string, Object>;
    }
    return undefined;
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
      invokeFunction: (functionCall: A2UIFunctionCall): A2UIValueType =>
        FunctionBridge.invoke(functionCall, surfaceId, componentId, renderId)
    } as A2UIResolveContext;
  }

  private resolveWeight(): number {
    let weight = this.attribute?.properties?.weight;
    if (weight !== undefined && weight > 0) {
      return weight;
    }
    return 0;
  }

  private resolveAccessibilityLabel(): string {
    return this.attribute?.properties?.accessibilityLabel ?? '';
  }

  private resolveAccessibilityDescription(): string {
    return this.attribute?.properties?.accessibilityDescription ?? '';
  }

  private resolveContentAreaBackgroundColor(): string {
    return this.attribute?.componentTheme?.colorMode === ThemeMode.DARK ?
      DEFAULT_TAB_CONTENT_AREA_BACKGROUND_COLOR_DARK :
      DEFAULT_TAB_CONTENT_AREA_BACKGROUND_COLOR_LIGHT;
  }
}

@Builder
export function extendedTabContentBuilder(attribute: CustomComponentAttribute): void {
  ExtendedTabContent({ attribute: attribute });
}

const extendedTabContentSchemaProvider: SchemaProvider = (_version: string): string => {
  return SchemaResourceLoader.loadSchema('schema/Extended/components/ExtendedTabContent.json');
};

export function createExtendedTabContentDefinition(): CustomComponentDefinition {
  return {
    type: EXTENDED_TAB_CONTENT_TYPE,
    builder: wrapBuilder(extendedTabContentBuilder),
    schemaProvider: extendedTabContentSchemaProvider
  };
}
