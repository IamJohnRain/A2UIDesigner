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

import { LengthMetrics } from '@kit.ArkUI';
import nativeEngine, {
  SyncComponentBoundDataModelResult,
  ValidateCustomComponentChecksResult
} from 'liba2ui_native.so';
import { CustomComponentAttribute, resolveComponentBrandColor } from './CustomComponent';
import { CustomComponentDefinition } from './CustomComponentFactory';
import {
  parseCustomProps,
  parseJsonArrayOptionValue,
  resolveCustomComponentWarningPathPrefixById,
  withAttributeSchemaWarningContext,
  resolveMargin,
  Margin,
  buildCustomPropsSignature
} from './CustomComponentUtils';
import { SchemaErrorCode, SchemaErrorInfoManager } from '../../schema/SchemaErrorInfoManager';
import { SchemaLocalPropertyHelper } from '../../schema/SchemaLocalPropertyHelper';
import { SchemaResourceLoader } from '../../schema/SchemaResourceLoader';
import { A2UIDynamicString } from '../../types/DynamicString';
import { A2UIResolveContext, ResolveResult } from '../../types/runtime/ResolveContext';
import { DynamicValueResolver } from '../../types/runtime/DynamicValueResolver';
import { ThemeMode } from '../../base/NativeEngineBridge';
import {
  A2UIValueType,
  SchemaProvider
} from '../../types/types';
import { CatalogItem } from '../../base/CatalogItem';

const CHOICE_PICKER_TYPE: string = 'ChoicePicker';
const CHOICE_PICKER_BINDINGS_KEY: string = '__a2uiBindings';
const CHOICE_PICKER_VARIANT_MULTIPLE_SELECTION: string = 'multipleSelection';
const CHOICE_PICKER_VARIANT_MUTUALLY_EXCLUSIVE: string = 'mutuallyExclusive';
const CHOICE_PICKER_VARIANT_LEGACY_MULTIPLE: string = 'multiple';
const CHOICE_PICKER_VARIANT_LEGACY_SINGLE: string = 'single';
const CHOICE_PICKER_VALUE_BINDING_MODE_ITEM: string = 'item';
const CHOICE_PICKER_VALUE_BINDING_MODE_LIST: string = 'list';
const CHOICE_PICKER_PANEL_SPACE_RESOURCE: Resource = $r('app.float.custom_choice_picker_panel_space');
const CHOICE_PICKER_LABEL_FONT_SIZE_RESOURCE: Resource = $r('app.float.custom_choice_picker_label_font_size');
const CHOICE_PICKER_ERROR_FONT_SIZE_RESOURCE: Resource = $r('app.float.custom_choice_picker_error_font_size');
const CHOICE_PICKER_OPTION_FONT_SIZE_RESOURCE: Resource = $r('app.float.custom_choice_picker_option_font_size');
const CHOICE_PICKER_OPTION_TEXT_MARGIN_RESOURCE: Resource = $r('app.float.custom_choice_picker_option_text_margin');
const CHOICE_PICKER_CHIP_HORIZONTAL_PADDING_RESOURCE: Resource =
  $r('app.float.custom_choice_picker_chip_horizontal_padding');
const CHOICE_PICKER_CHIP_VERTICAL_PADDING_RESOURCE: Resource = $r('app.float.custom_choice_picker_chip_vertical_padding');
const CHOICE_PICKER_CHIP_RADIUS_RESOURCE: Resource = $r('app.float.custom_choice_picker_chip_radius');
const CHOICE_PICKER_ERROR_COLOR_RESOURCE: Resource = $r('app.color.custom_error_text');
const CHOICE_PICKER_SELECTED_BACKGROUND_RESOURCE: Resource = $r('app.color.custom_button_confirm_background');
const CHOICE_PICKER_UNSELECTED_BACKGROUND_RESOURCE: Resource = $r('app.color.custom_choice_picker_unselected_background');
const CHOICE_PICKER_DARK_UNSELECTED_BACKGROUND_RESOURCE: Resource = $r('app.color.custom_choice_dark_picker_unselected_background');
const CHOICE_PICKER_SELECTED_TEXT_COLOR_RESOURCE: Resource = $r('app.color.custom_text_inverse');
const CHOICE_PICKER_UNSELECTED_TEXT_COLOR_RESOURCE: Resource = $r('app.color.custom_text_primary');

const CHOICE_PICKER_VARIANT_ALLOWED: string[] = [
  CHOICE_PICKER_VARIANT_MUTUALLY_EXCLUSIVE,
  CHOICE_PICKER_VARIANT_MULTIPLE_SELECTION
];
const CHOICE_PICKER_VARIANT_ALIASES: Map<string, string> = new Map<string, string>([
  [CHOICE_PICKER_VARIANT_LEGACY_MULTIPLE, CHOICE_PICKER_VARIANT_MULTIPLE_SELECTION],
  [CHOICE_PICKER_VARIANT_LEGACY_SINGLE, CHOICE_PICKER_VARIANT_MUTUALLY_EXCLUSIVE]
]);
const CHOICE_PICKER_DISPLAY_STYLE_ALLOWED: string[] = ['checkbox', 'chips'];
const DEFAULT_DISPLAY_WIDTH_VP: number = 360;
const DEFAULT_DISPLAY_HEIGHT_VP: number = 640;
const INLINE_OPTIONS_MAX_HEIGHT_VP: number = 220;
const INLINE_OPTIONS_SCREEN_RATIO: number = 0.28;
const INLINE_OPTION_ESTIMATED_HEIGHT_VP: number = 44;

interface ChoicePickerOption {
  label: string;
  value: string;
}

interface ChoicePickerOptions {
  label?: string;
  variant?: string;
  options: ChoicePickerOption[];
  value: string[];
  valuePath?: string;
  valueBindingMode?: string;
  displayStyle?: string;
  filterable?: boolean;
}

interface DynamicStringPathOption {
  path: string;
}

interface ChoicePickerValidationResult {
  valid: boolean;
  message: string;
}

function createDefaultChoicePickerOptions(): ChoicePickerOptions {
  return {
    variant: CHOICE_PICKER_VARIANT_MUTUALLY_EXCLUSIVE,
    options: [],
    value: [],
    displayStyle: 'checkbox',
    filterable: false
  };
}

function resolveChoicePickerDynamicString(
  value: Object | undefined,
  context?: A2UIResolveContext
): string | undefined {
  if (value === undefined) {
    return undefined;
  }
  let resolvedValue: string = DynamicValueResolver.resolveString(value as A2UIDynamicString, '', context);
  return resolvedValue.length > 0 ? resolvedValue : undefined;
}

function recordChoicePickerWarning(code: SchemaErrorCode, message: string, path: string): void {
  SchemaErrorInfoManager.getInstance().recordSchemaWarning(code, message, path, 'component', 'ChoicePicker');
}

function resolveChoicePickerWarningPathPrefix(componentId: string | undefined): string {
  return resolveCustomComponentWarningPathPrefixById(componentId);
}

function resolveChoicePickerWarningPath(pathPrefix: string, key: string): string {
  return `${pathPrefix}.${key}`;
}

function resolveChoicePickerOptionPath(pathPrefix: string, index: number): string {
  return `${pathPrefix}.options[${index}]`;
}

function isChoicePickerRecordValue(value: Object | undefined): boolean {
  return typeof value === 'object' && value !== null && !Array.isArray(value);
}

function normalizeChoicePickerValueList(value: A2UIValueType | undefined): string[] | undefined {
  if (Array.isArray(value)) {
    return value.map((item: A2UIValueType): string => {
      return typeof item === 'string' ? item : '';
    }).filter((item: string) => item.length > 0);
  }
  if (typeof value === 'string') {
    return value.length > 0 ? [value] : [];
  }
  return undefined;
}

function resolveChoicePickerDynamicValueList(
  value: Object | undefined,
  context?: A2UIResolveContext
): string[] | undefined {
  if (value === undefined) {
    return undefined;
  }
  let result = DynamicValueResolver.resolveValue(value as A2UIDynamicString, context);
  if (!result.success) {
    return undefined;
  }
  return normalizeChoicePickerValueList(result.value);
}

function resolveChoicePickerValueListByPath(
  path: string | undefined,
  context?: A2UIResolveContext
): string[] | undefined {
  if (path === undefined || path.length === 0) {
    return undefined;
  }
  let pathValue: DynamicStringPathOption = { path: path };
  return resolveChoicePickerDynamicValueList(pathValue as Object, context);
}

function parseChoicePickerArrayOptionValue(
  value: Object | undefined,
  field: string
): Object[] | undefined {
  if (Array.isArray(value)) {
    return value as Object[];
  }
  if (typeof value === 'string' && value.trim().startsWith('[')) {
    return parseJsonArrayOptionValue(value, 'CustomChoicePicker', field);
  }
  return undefined;
}

function resolveDynamicStringPath(value: Object | undefined): string | undefined {
  if (typeof value !== 'object' || value === null || Array.isArray(value)) {
    return undefined;
  }
  let valueObject = value as Record<string, Object>;
  let pathValue: Object | undefined = valueObject['path'];
  return typeof pathValue === 'string' && pathValue.length > 0 ? pathValue : undefined;
}

function resolveChoicePickerArrayItemBindingPath(value: Object | undefined): string | undefined {
  if (!Array.isArray(value) || value.length !== 1) {
    return undefined;
  }
  return resolveDynamicStringPath(value[0] as Object);
}

function resolveChoicePickerBindingPath(parsed: Record<string, Object>, field: string): string | undefined {
  let bindingsValue: Object | undefined = parsed[CHOICE_PICKER_BINDINGS_KEY];
  if (typeof bindingsValue === 'object' && bindingsValue !== null && !Array.isArray(bindingsValue)) {
    let bindings = bindingsValue as Record<string, Object>;
    let pathValue: Object | undefined = bindings[field];
    if (typeof pathValue === 'string' && pathValue.length > 0) {
      return pathValue;
    }
  }
  return resolveDynamicStringPath(parsed[field]) ?? resolveChoicePickerArrayItemBindingPath(parsed[field]);
}

function resolveChoicePickerValueBindingMode(
  value: Object | undefined,
  valuePath: string | undefined
): string | undefined {
  if (valuePath === undefined) {
    return undefined;
  }
  if (Array.isArray(value)) {
    return resolveChoicePickerArrayItemBindingPath(value) !== undefined
      ? CHOICE_PICKER_VALUE_BINDING_MODE_ITEM
      : CHOICE_PICKER_VALUE_BINDING_MODE_LIST;
  }
  if (typeof value === 'string') {
    return CHOICE_PICKER_VALUE_BINDING_MODE_ITEM;
  }
  return CHOICE_PICKER_VALUE_BINDING_MODE_LIST;
}

function resolveChoicePickerValues(
  value: Object | undefined,
  valuePath: string | undefined,
  context?: A2UIResolveContext
): string[] | undefined {
  let arrayValue = parseChoicePickerArrayOptionValue(value, 'value');
  if (arrayValue !== undefined) {
    return arrayValue.map((item: Object): string => {
      return resolveChoicePickerDynamicString(item, context) ?? '';
    }).filter((item: string) => item.length > 0);
  }

  if (typeof value === 'string') {
    return valuePath !== undefined && value.length > 0 ? [value] : undefined;
  }

  let dynamicValues: string[] | undefined = resolveChoicePickerDynamicValueList(value, context);
  if (dynamicValues !== undefined) {
    return dynamicValues;
  }

  return resolveChoicePickerValueListByPath(valuePath, context);
}

function normalizeChoicePickerArrayProperty(
  source: Record<string, Object>,
  key: string,
  required: boolean = false,
  pathPrefix: string = 'customProps'
): Object[] {
  const rawValue: Object | undefined = source[key];
  const propertyPath: string = resolveChoicePickerWarningPath(pathPrefix, key);
  if (rawValue === undefined) {
    if (required) {
      recordChoicePickerWarning(SchemaErrorCode.REQUIRED_MISS, `Property ${key} is required`, propertyPath);
    }
    return [];
  }

  if (Array.isArray(rawValue)) {
    return rawValue as Object[];
  }

  if (typeof rawValue === 'string') {
    let parsedArray: Object[] | undefined = parseJsonArrayOptionValue(rawValue, 'CustomChoicePicker', key);
    if (parsedArray !== undefined) {
      recordChoicePickerWarning(
        SchemaErrorCode.TYPE_MISMATCH,
        `Property ${key} expects array, stringified JSON array has been coerced`,
        propertyPath
      );
      return parsedArray;
    }
  }

  recordChoicePickerWarning(
    SchemaErrorCode.TYPE_MISMATCH,
    `Property ${key} expects array, fallback to empty array`,
    propertyPath
  );
  return [];
}

function resolveChoicePickerDynamicStringProperty(
  source: Record<string, Object>,
  key: string,
  context?: A2UIResolveContext,
  required: boolean = false,
  pathPrefix: string = 'customProps'
): string | undefined {
  const rawValue: Object | undefined = source[key];
  const propertyPath: string = `${pathPrefix}.${key}`;
  if (rawValue === undefined) {
    if (required) {
      recordChoicePickerWarning(SchemaErrorCode.REQUIRED_MISS, `Property ${key} is required`, propertyPath);
    }
    return undefined;
  }

  if (typeof rawValue === 'string') {
    return rawValue;
  }

  if (isChoicePickerRecordValue(rawValue)) {
    let rawRecord: Record<string, Object> = rawValue as Record<string, Object>;
    if (typeof rawRecord['path'] === 'string' || typeof rawRecord['call'] === 'string') {
      return resolveChoicePickerDynamicString(rawValue, context);
    }

    if (typeof rawRecord['value'] === 'string') {
      recordChoicePickerWarning(
        SchemaErrorCode.TYPE_MISMATCH,
        `Property ${key} expects DynamicString, object.value has been coerced`,
        propertyPath
      );
      return rawRecord['value'] as string;
    }
  }

  recordChoicePickerWarning(
    SchemaErrorCode.TYPE_MISMATCH,
    `Property ${key} expects DynamicString compatible value, fallback to default value`,
    propertyPath
  );
  return undefined;
}

function normalizeChoicePickerOption(
  item: Record<string, Object>,
  index: number,
  context?: A2UIResolveContext,
  pathPrefix: string = 'customProps'
): ChoicePickerOption {
  let optionPath: string = resolveChoicePickerOptionPath(pathPrefix, index);
  SchemaLocalPropertyHelper.recordUnknownFields(item, ['label', 'value'], 'ChoicePicker', optionPath);
  let labelValue: string | undefined =
    resolveChoicePickerDynamicStringProperty(item, 'label', context, true, optionPath);
  let valueValue: string | undefined =
    resolveChoicePickerDynamicStringProperty(item, 'value', context, true, optionPath);
  return {
    label: labelValue ?? '',
    value: valueValue ?? ''
  };
}

function normalizeChoicePickerDynamicStringArray(
  source: Record<string, Object>,
  key: string,
  context?: A2UIResolveContext,
  required: boolean = false,
  pathPrefix: string = 'customProps'
): string[] {
  let rawItems: Object[] = normalizeChoicePickerArrayProperty(source, key, required, pathPrefix);
  let normalizedValues: string[] = [];
  for (let i = 0; i < rawItems.length; i++) {
    let currentValue: Object | undefined = rawItems[i];
    if (typeof currentValue !== 'string' && !isChoicePickerRecordValue(currentValue)) {
      recordChoicePickerWarning(
        SchemaErrorCode.TYPE_MISMATCH,
        `Property ${key}[${i}] expects DynamicString compatible item, drop current item`,
        `${resolveChoicePickerWarningPath(pathPrefix, key)}[${i}]`
      );
      continue;
    }
    let resolvedValue: string | undefined = resolveChoicePickerDynamicString(currentValue, context);
    if (resolvedValue !== undefined) {
      normalizedValues.push(resolvedValue);
    }
  }
  return normalizedValues;
}

function resolveChoicePickerVariantMode(variant: string | undefined): string {
  if (variant === CHOICE_PICKER_VARIANT_MUTUALLY_EXCLUSIVE) {
    return CHOICE_PICKER_VARIANT_MUTUALLY_EXCLUSIVE;
  }
  if (variant === CHOICE_PICKER_VARIANT_LEGACY_SINGLE) {
    return CHOICE_PICKER_VARIANT_MUTUALLY_EXCLUSIVE;
  }
  if (variant === CHOICE_PICKER_VARIANT_MULTIPLE_SELECTION) {
    return CHOICE_PICKER_VARIANT_MULTIPLE_SELECTION;
  }
  if (variant === CHOICE_PICKER_VARIANT_LEGACY_MULTIPLE) {
    return CHOICE_PICKER_VARIANT_MULTIPLE_SELECTION;
  }
  return CHOICE_PICKER_VARIANT_MUTUALLY_EXCLUSIVE;
}

export function resolveChoicePickerChipBackgroundColor(
  isSelected: boolean,
  hasSelectedColor: boolean,
  selectedColor: ResourceColor,
  isDarkMode: boolean
): ResourceColor {
  if (!isSelected) {
    return isDarkMode ? CHOICE_PICKER_DARK_UNSELECTED_BACKGROUND_RESOURCE : CHOICE_PICKER_UNSELECTED_BACKGROUND_RESOURCE;
  }
  if (hasSelectedColor) {
    return selectedColor;
  }
  return CHOICE_PICKER_SELECTED_BACKGROUND_RESOURCE;
}

export function resolveChoicePickerChipTextColor(
  isSelected: boolean,
  isDarkMode: boolean
): Resource {
  if (isDarkMode) {
    return isSelected ? CHOICE_PICKER_UNSELECTED_TEXT_COLOR_RESOURCE : CHOICE_PICKER_SELECTED_TEXT_COLOR_RESOURCE;
  }
  return isSelected ? CHOICE_PICKER_SELECTED_TEXT_COLOR_RESOURCE : CHOICE_PICKER_UNSELECTED_TEXT_COLOR_RESOURCE;
}

@Component
struct ChoicePickerSelectionPanel {
  @Prop options: ChoicePickerOptions = createDefaultChoicePickerOptions();
  @Prop selectedValues: string[] = [];
  @Prop validationMessage: string = '';
  @Prop checkboxGroupName: string = 'choice-picker-inline-checkbox';
  @Prop hasSelectedColor: boolean = false;
  @Prop selectedColor: ResourceColor = 0;
  @Prop isDarkMode: boolean = false;
  onSelectionChange: (values: string[]) => void = (_values: string[]) => {
  };
  @State searchText: string = '';

  aboutToAppear(): void {
    this.searchText = '';
  }

  build() {
    Column() {
      if (this.options.filterable === true) {
        TextInput({ text: this.searchText})
          .width('100%')
          .margin({ bottom: CHOICE_PICKER_PANEL_SPACE_RESOURCE })
          .onChange((value: string) => {
            this.searchText = value;
          });
      }

      Flex({ direction: FlexDirection.Row, wrap: FlexWrap.Wrap, space: { main: LengthMetrics.resource(CHOICE_PICKER_PANEL_SPACE_RESOURCE), cross: LengthMetrics.resource(CHOICE_PICKER_PANEL_SPACE_RESOURCE) } }) {
        if (this.getFilteredOptions().length > 0) {
          ForEach(this.getFilteredOptions(), (option: ChoicePickerOption) => {
            if (this.options.displayStyle === 'chips') {
              this.ChipOption(option);
            } else {
              this.CheckboxOption(option);
            }
          }, (option: ChoicePickerOption) => option.value);
        }
      }
      .width('100%');

      if (this.validationMessage.length > 0) {
        Text(this.validationMessage)
          .width('100%')
          .fontSize(CHOICE_PICKER_ERROR_FONT_SIZE_RESOURCE)
          .fontColor(CHOICE_PICKER_ERROR_COLOR_RESOURCE);
      }
    }
    .width('100%')
    .alignItems(HorizontalAlign.Start);
  }

  @Builder
  private CheckboxOption(option: ChoicePickerOption) {
    Row() {
      if (this.hasSelectedColor) {
        Checkbox({ name: option.value, group: this.checkboxGroupName })
          .shape(CheckBoxShape.ROUNDED_SQUARE)
          .select(this.selectedValues.includes(option.value))
          .selectedColor(this.selectedColor)
          .hitTestBehavior(HitTestMode.None)
      } else {
        Checkbox({ name: option.value, group: this.checkboxGroupName })
          .shape(CheckBoxShape.ROUNDED_SQUARE)
          .select(this.selectedValues.includes(option.value))
          .hitTestBehavior(HitTestMode.None)
      }
      Text(option.label)
        .fontSize(CHOICE_PICKER_OPTION_FONT_SIZE_RESOURCE)
        .margin({ left: CHOICE_PICKER_OPTION_TEXT_MARGIN_RESOURCE })
        .layoutWeight(1)
        .maxLines(1)
        .textOverflow({ overflow: TextOverflow.Ellipsis });
    }
    .width('100%')
    .alignItems(VerticalAlign.Center)
    .padding({ top: CHOICE_PICKER_PANEL_SPACE_RESOURCE, bottom: CHOICE_PICKER_PANEL_SPACE_RESOURCE })
    .onClick(() => {
      const isSelected = this.selectedValues.includes(option.value);
      this.onSelectionChange(this.buildNextSelectedValues(option.value, !isSelected));
    });
  }

  @Builder
  private ChipOption(option: ChoicePickerOption) {
    Row() {
      Text(option.label)
        .fontSize(CHOICE_PICKER_OPTION_FONT_SIZE_RESOURCE)
        .textAlign(TextAlign.Start)
        .maxLines(1)
        .textOverflow({ overflow: TextOverflow.Ellipsis })
        .padding({
          left: CHOICE_PICKER_CHIP_HORIZONTAL_PADDING_RESOURCE,
          right: CHOICE_PICKER_CHIP_HORIZONTAL_PADDING_RESOURCE,
          top: CHOICE_PICKER_CHIP_VERTICAL_PADDING_RESOURCE,
          bottom: CHOICE_PICKER_CHIP_VERTICAL_PADDING_RESOURCE
        })
        .borderRadius(CHOICE_PICKER_CHIP_RADIUS_RESOURCE)
        .backgroundColor(resolveChoicePickerChipBackgroundColor(
          this.selectedValues.includes(option.value),
          this.hasSelectedColor,
          this.selectedColor,
          this.isDarkMode
        ))
        .fontColor(resolveChoicePickerChipTextColor(
          this.selectedValues.includes(option.value),
          this.isDarkMode
        ));
    }
    .onClick(() => {
      const isSelected = this.selectedValues.includes(option.value);
      this.onSelectionChange(this.buildNextSelectedValues(option.value, !isSelected));
    });
  }

  private isMultipleSelection(): boolean {
    return resolveChoicePickerVariantMode(this.options.variant) === CHOICE_PICKER_VARIANT_MULTIPLE_SELECTION;
  }

  private getFilteredOptions(): ChoicePickerOption[] {
    if (this.searchText.length === 0) {
      return this.options.options;
    }

    return this.options.options.filter((option: ChoicePickerOption) =>
    option.label.toLowerCase().includes(this.searchText.toLowerCase()) ||
    option.value.toLowerCase().includes(this.searchText.toLowerCase())
    );
  }

  private buildNextSelectedValues(value: string, isSelected: boolean): string[] {
    if (this.isMultipleSelection()) {
      if (isSelected) {
        if (this.selectedValues.includes(value)) {
          return [...this.selectedValues];
        }
        return [...this.selectedValues, value];
      }
      return this.selectedValues.filter((item: string) => item !== value);
    }

    return isSelected ? [value] : [];
  }
}

@Component
export struct CustomChoicePicker {
  @Prop @Watch('onAttributeChanged') attribute: CustomComponentAttribute | undefined = undefined;
  @State selectedValues: string[] = [];
  @State selectedValuesSource: string = '';
  @State attributeRevision: number = 0;
  @State validationMessage: string = '';
  private resolvedOptions: ChoicePickerOptions = createDefaultChoicePickerOptions();
  private hasResolvedOptions: boolean = false;
  private lastResolvedSignature: string = '';

  private onAttributeChanged(): void {
    let resolvedOptions: ChoicePickerOptions = this.resolveOptions();
    if (!this.shouldRenderChoicePicker(resolvedOptions)) {
      this.refreshSelectedValues(resolvedOptions);
      this.validationMessage = '';
      this.attributeRevision += 1;
      return;
    }
    this.refreshSelectedValues(resolvedOptions);
    this.validationMessage = this.resolveValidationMessage(this.resolveCommittedValues(resolvedOptions));
    this.attributeRevision += 1;
  }

  aboutToAppear(): void {
    let resolvedOptions: ChoicePickerOptions = this.resolveOptions();
    if (!this.shouldRenderChoicePicker(resolvedOptions)) {
      this.refreshSelectedValues(resolvedOptions);
      this.validationMessage = '';
      return;
    }
    this.refreshSelectedValues(resolvedOptions);
    this.validationMessage = this.resolveValidationMessage(this.resolveCommittedValues(resolvedOptions));
  }

  build() {
    Column() {
      if (this.shouldRenderChoicePicker()) {
        Column({ space: 8 }) {
          this.ChoicePickerContent();
        }
      }
    }
    .alignItems(HorizontalAlign.Start)
    .layoutWeight(this.resolveWeight())
    .margin(this.resolveMargin())
    .accessibilityGroup(true)
    .accessibilityText(this.resolveAccessibilityLabel())
    .accessibilityDescription(this.resolveAccessibilityDescription())
    .id(this.attribute?.id ?? '');
  }

  @Builder
  private ChoicePickerContent() {
    if (this.hasChoicePickerLabel()) {
      Text(this.resolveChoicePickerLabel())
        .width('100%')
        .fontSize(CHOICE_PICKER_LABEL_FONT_SIZE_RESOURCE)
        .fontWeight(FontWeight.Medium)
        .textAlign(TextAlign.Start)
        .maxLines(1)
        .textOverflow({ overflow: TextOverflow.Ellipsis });
    }

    ChoicePickerSelectionPanel({
      options: this.resolveOptions(this.attributeRevision),
      selectedValues: this.resolveCommittedValues(),
      validationMessage: this.validationMessage,
      checkboxGroupName: this.resolveCheckboxGroupName(),
      hasSelectedColor: this.hasBrandColor(),
      selectedColor: this.resolveBrandColor(),
      isDarkMode: this.attribute?.componentTheme?.colorMode === ThemeMode.DARK,
      onSelectionChange: (values: string[]) => {
        this.applySelection(values);
      }
    });
  }

  private hasChoicePickerLabel(options?: ChoicePickerOptions): boolean {
    return this.resolveChoicePickerLabel(options).length > 0;
  }

  private resolveChoicePickerLabel(options?: ChoicePickerOptions): string {
    let resolvedOptions: ChoicePickerOptions = options ?? this.resolveOptions(this.attributeRevision);
    return resolvedOptions.label ?? '';
  }

  private applySelection(values: string[]): void {
    let resolvedOptions: ChoicePickerOptions = this.resolveOptions(this.attributeRevision);
    this.selectedValues = [...values];
    this.selectedValuesSource = this.serializeSelectedValues(resolvedOptions.value);
    this.attributeRevision += 1;
    this.validationMessage = this.resolveValidationMessage(values);
    updateChoicePickerDataModelValue(
      this.attribute?.renderId,
      this.attribute?.surfaceId,
      this.attribute?.id,
      values,
      resolvedOptions.valueBindingMode
    );
  }


  private resolveWeight(): number {
    let weight = this.attribute?.properties?.weight;
    if (weight !== undefined && weight > 0) {
      return weight;
    }
    return 0;
  }

  private resolveMargin(): Margin | undefined {
    return resolveMargin(this.attribute);
  }

  private resolveAccessibilityLabel(): string {
    return this.attribute?.properties?.accessibilityLabel ?? '';
  }

  private resolveAccessibilityDescription(): string {
    return this.attribute?.properties?.accessibilityDescription ?? '';
  }

  private shouldRenderChoicePicker(options?: ChoicePickerOptions): boolean {
    let resolvedOptions: ChoicePickerOptions = options ?? this.resolveOptions(this.attributeRevision);
    return resolvedOptions.options.length > 0;
  }

  private resolveCommittedValues(options?: ChoicePickerOptions): string[] {
    let resolvedOptions: ChoicePickerOptions = options ?? this.resolveOptions(this.attributeRevision);
    let sourceValues: string = this.serializeSelectedValues(resolvedOptions.value);
    if (this.selectedValuesSource === sourceValues) {
      return [...this.selectedValues];
    }
    return [...resolvedOptions.value];
  }

  private refreshSelectedValues(options: ChoicePickerOptions): void {
    this.selectedValues = [...options.value];
    this.selectedValuesSource = this.serializeSelectedValues(options.value);
  }

  private serializeSelectedValues(values: string[]): string {
    return JSON.stringify(values);
  }

  private resolveValidationMessage(values: string[]): string {
    let validationResult: ChoicePickerValidationResult = validateChoicePickerChecks(
      this.attribute?.renderId,
      this.attribute?.surfaceId,
      this.attribute?.id,
      values
    );
    return validationResult.valid ? '' : validationResult.message;
  }

  private resolveCheckboxGroupName(): string {
    let componentId: string | undefined = this.attribute?.id;
    if (componentId !== undefined && componentId.length > 0) {
      return `choice-picker-inline-${componentId}`;
    }
    return 'choice-picker-inline-default';
  }

  private hasBrandColor(): boolean {
    return resolveComponentBrandColor(this.attribute) !== undefined;
  }

  private resolveBrandColor(): ResourceColor {
    return resolveComponentBrandColor(this.attribute) ?? 0;
  }

  private resolveOptions(_revision?: number): ChoicePickerOptions {
    this.ensureResolvedOptionsSynchronized();
    return this.resolvedOptions;
  }

  private ensureResolvedOptionsSynchronized(): void {
    let currentSignature: string = buildCustomPropsSignature(this.attribute);
    if (this.hasResolvedOptions && this.lastResolvedSignature === currentSignature) {
      return;
    }
    this.synchronizeResolvedOptions();
  }

  private synchronizeResolvedOptions(): void {
    this.resolvedOptions = this.resolveOptionsForSchemaWarning();
    this.hasResolvedOptions = true;
    this.lastResolvedSignature = buildCustomPropsSignature(this.attribute);
  }

  private resolveOptionsForSchemaWarning(): ChoicePickerOptions {
    return withAttributeSchemaWarningContext(this.attribute, (): ChoicePickerOptions => {
      let parsed = parseCustomProps(this.attribute, 'CustomChoicePicker');
      if (parsed === undefined) {
        return createDefaultChoicePickerOptions();
      }

      const warningPathPrefix: string = resolveChoicePickerWarningPathPrefix(this.attribute?.id);
      SchemaLocalPropertyHelper.recordUnknownFields(
        parsed,
        ['label', 'variant', 'options', 'value', 'displayStyle', 'filterable', CHOICE_PICKER_BINDINGS_KEY],
        'ChoicePicker',
        warningPathPrefix
      );
      let resolvedOptions: ChoicePickerOptions = createDefaultChoicePickerOptions();
      let resolveContext: A2UIResolveContext | undefined = this.createResolveContext();

      let resolvedLabel: string | undefined =
        resolveChoicePickerDynamicStringProperty(parsed, 'label', resolveContext, false, warningPathPrefix);
      if (resolvedLabel !== undefined) {
        resolvedOptions.label = resolvedLabel;
      }

      resolvedOptions.variant = SchemaLocalPropertyHelper.normalizeEnumStringProperty(
        parsed,
        'variant',
        CHOICE_PICKER_VARIANT_MUTUALLY_EXCLUSIVE,
        CHOICE_PICKER_VARIANT_ALLOWED,
        'ChoicePicker',
        warningPathPrefix,
        false,
        CHOICE_PICKER_VARIANT_ALIASES,
        CHOICE_PICKER_VARIANT_MULTIPLE_SELECTION
      );
      resolvedOptions.displayStyle = SchemaLocalPropertyHelper.normalizeEnumStringProperty(
        parsed,
        'displayStyle',
        'checkbox',
        CHOICE_PICKER_DISPLAY_STYLE_ALLOWED,
        'ChoicePicker',
        warningPathPrefix
      );
      resolvedOptions.filterable = SchemaLocalPropertyHelper.normalizeBooleanProperty(
        parsed,
        'filterable',
        false,
        'ChoicePicker',
        warningPathPrefix
      );

      let optionItems: Object[] = normalizeChoicePickerArrayProperty(parsed, 'options', true, warningPathPrefix);
      let normalizedOptions: ChoicePickerOption[] = [];
      optionItems.forEach((item: Object, index: number) => {
        if (!isChoicePickerRecordValue(item)) {
          recordChoicePickerWarning(
            SchemaErrorCode.TYPE_MISMATCH,
            `Property options[${index}] expects object item, drop current item`,
            resolveChoicePickerOptionPath(warningPathPrefix, index)
          );
          return;
        }
        let normalizedOption: ChoicePickerOption =
          normalizeChoicePickerOption(item as Record<string, Object>, index, resolveContext, warningPathPrefix);
        if (normalizedOption.label.length > 0 || normalizedOption.value.length > 0) {
          normalizedOptions.push(normalizedOption);
        }
      });
      resolvedOptions.options = normalizedOptions;

      let valuePath: string | undefined = resolveChoicePickerBindingPath(parsed, 'value');
      let rawValue: Object | undefined = parsed['value'];
      if (valuePath !== undefined) {
        let valueValue: string[] | undefined = resolveChoicePickerValues(rawValue, valuePath, resolveContext);
        if (valueValue !== undefined) {
          resolvedOptions.value = valueValue;
        }
        resolvedOptions.valuePath = valuePath;
        resolvedOptions.valueBindingMode = resolveChoicePickerValueBindingMode(rawValue, valuePath);
      } else if (rawValue === undefined) {
        // Preserve the current runtime gap: missing value does not emit a local required warning.
      } else if (isChoicePickerRecordValue(rawValue) && typeof (rawValue as Record<string, Object>)['call'] === 'string') {
        let valueValue: string[] | undefined = resolveChoicePickerValues(rawValue, undefined, resolveContext);
        if (valueValue !== undefined) {
          resolvedOptions.value = valueValue;
        } else {
          resolvedOptions.value = normalizeChoicePickerDynamicStringArray(
            parsed,
            'value',
            resolveContext,
            false,
            warningPathPrefix
          );
        }
      } else {
        resolvedOptions.value = normalizeChoicePickerDynamicStringArray(
          parsed,
          'value',
          resolveContext,
          false,
          warningPathPrefix
        );
      }

      return resolvedOptions;
    });
  }

  private createResolveContext(): A2UIResolveContext | undefined {
    if (this.attribute?.dataModel === undefined) {
      return undefined;
    }
    return {
      renderId: this.attribute.renderId,
      dataModel: this.attribute.dataModel,
      surfaceId: this.attribute.surfaceId,
      componentId: this.attribute.id
    } as A2UIResolveContext;
  }
}

function createChoicePickerValidationResult(valid: boolean, message: string): ChoicePickerValidationResult {
  let result: ChoicePickerValidationResult = {
    valid: valid,
    message: message
  };
  return result;
}

function validateChoicePickerChecks(
  renderId: number | undefined,
  surfaceId: string | undefined,
  componentId: string | undefined,
  selectedValues: string[]
): ChoicePickerValidationResult {
  if (renderId === undefined || surfaceId === undefined || surfaceId.length === 0 ||
    componentId === undefined || componentId.length === 0) {
    return createChoicePickerValidationResult(true, '');
  }

  try {
    let result: ValidateCustomComponentChecksResult | undefined = nativeEngine.validateCustomComponentChecks(
      renderId,
      surfaceId,
      componentId,
      JSON.stringify(selectedValues)
    );
    if (result === undefined) {
      console.warn('CustomChoicePicker validateChecks returned undefined');
      return createChoicePickerValidationResult(true, '');
    }
    return createChoicePickerValidationResult(
      result.valid,
      result.message
    );
  } catch (error) {
    console.error(`CustomChoicePicker validateChecks failed: ${JSON.stringify(error)}`);
    return createChoicePickerValidationResult(true, '');
  }
}

function updateChoicePickerDataModelValue(
  renderId: number | undefined,
  surfaceId: string | undefined,
  componentId: string | undefined,
  selectedValues: string[],
  valueBindingMode: string | undefined
): boolean {
  if (renderId === undefined || componentId === undefined || componentId.length === 0) {
    return false;
  }

  let valueToSync: string[] | string = valueBindingMode === CHOICE_PICKER_VALUE_BINDING_MODE_ITEM
    ? (selectedValues[0] ?? '')
    : selectedValues;
  try {
    let result: SyncComponentBoundDataModelResult = nativeEngine.syncComponentBoundDataModel(
      renderId,
      surfaceId ?? '',
      componentId,
      'value',
      JSON.stringify(valueToSync)
    );
    if (result.success !== true) {
      console.warn(`CustomChoicePicker syncComponentBoundDataModel failed: ${result.errorMessage ?? ''}`);
      return false;
    }
    return true;
  } catch (error) {
    console.error(`CustomChoicePicker syncComponentBoundDataModel failed: ${JSON.stringify(error)}`);
    return false;
  }
}

@Builder
export function customChoicePickerBuilder(attribute: CustomComponentAttribute): void {
  CustomChoicePicker({ attribute: attribute });
}

export function createChoicePickerDefinition(): CustomComponentDefinition {
  const schemaProvider: SchemaProvider = (version: string) => {
    return SchemaResourceLoader.loadA2UISchema(version, 'components/ChoicePicker.json');
  };
  return {
    type: CHOICE_PICKER_TYPE,
    builder: wrapBuilder(customChoicePickerBuilder),
    schemaProvider: schemaProvider
  };
}

export function asCatalogItem(): CatalogItem {
  const definition: CustomComponentDefinition = createChoicePickerDefinition();
  return CatalogItem.forComponent(
    definition.type,
    definition.schemaProvider ?? ((_version: string): string => ''),
    definition.builder
  ).markInnerNative(false);
}
