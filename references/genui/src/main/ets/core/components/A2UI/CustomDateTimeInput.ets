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
  SyncComponentBoundDataModelResult,
  ValidateCustomComponentChecksResult
} from 'liba2ui_native.so';
import { ComponentContent, UIContext } from '@kit.ArkUI';
import { CustomComponentAttribute, resolveComponentBrandColor } from './CustomComponent';
import { CustomComponentDefinition } from './CustomComponentFactory';
import {
  closeCustomDialogSafely,
  parseCustomProps,
  resolveCustomComponentWarningPathPrefix,
  resolveDynamicStringOptionValue,
  resolveDynamicStringPathOptionValue,
  resolveDialogUIContext,
  showCustomDialogSafely,
  withAttributeSchemaWarningContext,
  resolveMargin,
  Margin,
  buildCustomPropsSignature
} from './CustomComponentUtils';
import { SchemaErrorCode, SchemaErrorInfoManager } from '../../schema/SchemaErrorInfoManager';
import { SchemaResourceLoader } from '../../schema/SchemaResourceLoader';
import { SchemaLocalPropertyHelper } from '../../schema/SchemaLocalPropertyHelper';
import { SchemaProvider } from '../../types/types';
import { CatalogItem } from '../../base/CatalogItem';
import { ThemeMode } from '../../base/NativeEngineBridge';

const DATA_TIME_INPUT_TYPE: string = 'DateTimeInput';
const DATE_TIME_INPUT_BINDINGS_KEY: string = '__a2uiBindings';
const DATE_TIME_INPUT_SECTION_SPACE_RESOURCE: Resource = $r('app.float.custom_date_time_input_section_space');
const DATE_TIME_INPUT_CONTAINER_SPACE_RESOURCE: Resource = $r('app.float.custom_date_time_input_container_space');
const DATE_TIME_INPUT_DIALOG_TITLE_FONT_SIZE_RESOURCE: Resource =
  $r('app.float.custom_date_time_input_dialog_title_font_size');
const DATE_TIME_INPUT_DIALOG_TITLE_PADDING_RESOURCE: Resource =
  $r('app.float.custom_date_time_input_dialog_title_padding');
const DATE_TIME_INPUT_ERROR_FONT_SIZE_RESOURCE: Resource = $r('app.float.custom_date_time_input_error_font_size');
const DATE_TIME_INPUT_DIALOG_BUTTON_SPACE_RESOURCE: Resource = $r('app.float.custom_date_time_input_dialog_button_space');
const DATE_TIME_INPUT_PICKER_MIN_HEIGHT: number = 216;
const DATE_TIME_INPUT_DIALOG_PADDING_RESOURCE: Resource = $r('app.float.custom_date_time_input_dialog_padding');
const DATE_TIME_INPUT_DIALOG_RADIUS_RESOURCE: Resource = $r('app.float.custom_date_time_input_dialog_radius');
const DATE_TIME_INPUT_TEXT_FONT_SIZE_RESOURCE: Resource = $r('app.float.custom_date_time_input_text_font_size');
const DATE_TIME_INPUT_HORIZONTAL_PADDING_RESOURCE: Resource =
  $r('app.float.custom_date_time_input_horizontal_padding');
const DATE_TIME_INPUT_VERTICAL_PADDING_RESOURCE: Resource = $r('app.float.custom_date_time_input_vertical_padding');
const DATE_TIME_INPUT_RADIUS_RESOURCE: Resource = $r('app.float.custom_date_time_input_radius');
const DATE_TIME_INPUT_BORDER_WIDTH_RESOURCE: Resource = $r('app.float.custom_date_time_input_border_width');
const DATE_TIME_INPUT_ERROR_COLOR_RESOURCE: Resource = $r('app.color.custom_error_text');
const DATE_TIME_INPUT_BUTTON_CANCEL_BACKGROUND_RESOURCE: Resource = $r('app.color.custom_button_cancel_background');
const DATE_TIME_INPUT_TEXT_PRIMARY_COLOR_RESOURCE: Resource = $r('app.color.custom_text_primary');
const DATE_TIME_INPUT_BUTTON_CONFIRM_BACKGROUND_RESOURCE: Resource = $r('app.color.custom_button_confirm_background');
const DATE_TIME_INPUT_TEXT_INVERSE_COLOR_RESOURCE: Resource = $r('app.color.custom_text_inverse');
const DATE_TIME_INPUT_BACKGROUND_COLOR_RESOURCE: Resource = $r('app.color.custom_background');
const DATE_TIME_INPUT_DARK_BACKGROUND_COLOR_RESOURCE: Resource = $r('app.color.custom_darkBackground');
const DATE_TIME_INPUT_PLACEHOLDER_COLOR_RESOURCE: Resource = $r('app.color.custom_text_placeholder');
const DATE_TIME_INPUT_BORDER_ENABLED_COLOR_RESOURCE: Resource = $r('app.color.custom_border_enabled');
const DATE_TIME_INPUT_BORDER_DISABLED_COLOR_RESOURCE: Resource = $r('app.color.custom_border_disabled');
const DATE_TIME_INPUT_CANCEL_TEXT_RESOURCE: Resource = $r('app.string.custom_date_time_input_cancel');
const DATE_TIME_INPUT_CONFIRM_TEXT_RESOURCE: Resource = $r('app.string.custom_date_time_input_confirm');
const DATE_TIME_INPUT_DIALOG_TITLE_DATE_TIME_RESOURCE: Resource =
  $r('app.string.custom_date_time_input_dialog_title_date_time');
const DATE_TIME_INPUT_DIALOG_TITLE_DATE_RESOURCE: Resource = $r('app.string.custom_date_time_input_dialog_title_date');
const DATE_TIME_INPUT_DIALOG_TITLE_TIME_RESOURCE: Resource = $r('app.string.custom_date_time_input_dialog_title_time');
const DATE_TIME_INPUT_DIALOG_TITLE_DEFAULT_RESOURCE: Resource =
  $r('app.string.custom_date_time_input_dialog_title_default');
const DATE_TIME_INPUT_PLACEHOLDER_DATE_TIME_RESOURCE: Resource =
  $r('app.string.custom_date_time_input_placeholder_date_time');
const DATE_TIME_INPUT_PLACEHOLDER_DATE_RESOURCE: Resource = $r('app.string.custom_date_time_input_placeholder_date');
const DATE_TIME_INPUT_PLACEHOLDER_TIME_RESOURCE: Resource = $r('app.string.custom_date_time_input_placeholder_time');
const DATE_TIME_INPUT_PLACEHOLDER_DISABLED_RESOURCE: Resource =
  $r('app.string.custom_date_time_input_placeholder_disabled');

export interface DateTimeInputOptions {
  value?: string;
  valuePath?: string;
  enableDate?: boolean;
  enableTime?: boolean;
  min?: string;
  max?: string;
  label?: string;
}

interface DateTimeInputDialogAttribute {
  options: DateTimeInputOptions;
  initialValue: string;
  renderId?: number;
  surfaceId?: string;
  componentId?: string;
  colorMode?: ThemeMode;
  hasSelectedTextColor?: boolean;
  selectedTextColor?: ResourceColor;
  onCancelSelection: () => void;
  onConfirmSelection: (value: string) => void;
}

interface DateTimeInputValidationResult {
  valid: boolean;
  message: string;
}

interface TimeValueParts {
  hour: number;
  minute: number;
}

interface DateTimeValueParts {
  datePart: string;
  timePart: string;
}

interface DateTimeInputDateRange {
  start: Date;
  end: Date;
}

interface TimePickerBoundary {
  selected: Date;
  start: Date;
  end: Date;
}

function createDefaultDateTimeInputOptions(): DateTimeInputOptions {
  return {
    value: '',
    enableDate: false,
    enableTime: false,
    min: '',
    max: '',
    label: ''
  };
}

function hasRecordedDateTimeInputWarning(code: SchemaErrorCode, message: string, path: string): boolean {
  let warnings = SchemaErrorInfoManager.getInstance().getWarnings();
  for (const warning of warnings) {
    if (warning.code === code &&
      warning.message === message &&
      warning.path === path &&
      warning.itemType === 'component' &&
      warning.itemName === 'DateTimeInput') {
      return true;
    }
  }
  return false;
}

function recordDateTimeInputWarning(code: SchemaErrorCode, message: string, path: string): void {
  if (hasRecordedDateTimeInputWarning(code, message, path)) {
    return;
  }
  SchemaErrorInfoManager.getInstance().recordSchemaWarning(code, message, path, 'component', 'DateTimeInput');
}

function resolveDateTimeInputWarningPathPrefix(attribute: CustomComponentAttribute | undefined): string {
  return resolveCustomComponentWarningPathPrefix(attribute);
}

function resolveDateTimeInputWarningPath(pathPrefix: string, key: string): string {
  return `${pathPrefix}.${key}`;
}

function resolveDateTimeInputDynamicStringProperty(
  source: Record<string, Object>,
  key: string,
  attribute: CustomComponentAttribute | undefined,
  required: boolean = false,
  pathPrefix: string = 'customProps'
): string | undefined {
  const rawValue: Object | undefined = source[key];
  const propertyPath: string = resolveDateTimeInputWarningPath(pathPrefix, key);
  if (rawValue === undefined) {
    if (required) {
      recordDateTimeInputWarning(SchemaErrorCode.REQUIRED_MISS, `Property ${key} is required`, propertyPath);
    }
    return undefined;
  }

  if (typeof rawValue === 'string') {
    return rawValue;
  }

  if (typeof rawValue === 'object' && rawValue !== null && !Array.isArray(rawValue)) {
    let rawRecord: Record<string, Object> = rawValue as Record<string, Object>;
    if (typeof rawRecord['path'] === 'string' || typeof rawRecord['call'] === 'string') {
      return resolveDynamicStringOptionValue(rawValue, attribute);
    }

    if (typeof rawRecord['value'] === 'string') {
      recordDateTimeInputWarning(
        SchemaErrorCode.TYPE_MISMATCH,
        `Property ${key} expects DynamicString, object.value has been coerced`,
        propertyPath
      );
      return rawRecord['value'] as string;
    }
  }

  recordDateTimeInputWarning(
    SchemaErrorCode.TYPE_MISMATCH,
    `Property ${key} expects DynamicString compatible value, fallback to default value`,
    propertyPath
  );
  return undefined;
}

function parseTimeParts(value: string): TimeValueParts | undefined {
  let trimmedValue: string = value.trim();
  if (trimmedValue.length === 0) {
    return undefined;
  }

  let parts: string[] = trimmedValue.split(':');
  if (parts.length < 2) {
    return undefined;
  }

  let hour: number = Number(parts[0]);
  let minute: number = Number(parts[1]);
  if (Number.isNaN(hour) || Number.isNaN(minute) ||
      hour < 0 || hour > 23 || minute < 0 || minute > 59) {
    return undefined;
  }

  return {
    hour: hour,
    minute: minute
  };
}

function isValidDatePart(year: number, month: number, day: number): boolean {
  if (Number.isNaN(year) || Number.isNaN(month) || Number.isNaN(day) ||
      month < 1 || month > 12 || day < 1 || day > 31) {
    return false;
  }

  let normalizedDate: Date = new Date(year, month - 1, day);
  return normalizedDate.getFullYear() === year &&
    normalizedDate.getMonth() === month - 1 &&
    normalizedDate.getDate() === day;
}

function splitDateTimeValue(value: string): DateTimeValueParts {
  let trimmedValue: string = value.trim();
  let separatorIndex: number = trimmedValue.indexOf('T');
  if (separatorIndex < 0) {
    separatorIndex = trimmedValue.indexOf(' ');
  }
  if (separatorIndex < 0) {
    return {
      datePart: trimmedValue,
      timePart: trimmedValue
    };
  }

  return {
    datePart: trimmedValue.substring(0, separatorIndex).trim(),
    timePart: trimmedValue.substring(separatorIndex + 1).trim()
  };
}

function parseDateValue(value: string | undefined): Date | undefined {
  if (value === undefined || value.length === 0) {
    return undefined;
  }

  let dateTimeParts: DateTimeValueParts = splitDateTimeValue(value);

  let dateParts: string[] = dateTimeParts.datePart.split('-');
  if (dateParts.length < 3) {
    return undefined;
  }

  let year: number = Number(dateParts[0]);
  let month: number = Number(dateParts[1]);
  let day: number = Number(dateParts[2]);
  if (!isValidDatePart(year, month, day)) {
    return undefined;
  }

  let hour: number = 0;
  let minute: number = 0;
  let timeValue: TimeValueParts | undefined = parseTimeParts(dateTimeParts.timePart);
  if (timeValue !== undefined) {
    hour = timeValue.hour;
    minute = timeValue.minute;
  }
  return new Date(year, month - 1, day, hour, minute);
}

function parseTimeValue(value: string | undefined): Date | undefined {
  if (value === undefined || value.length === 0) {
    return undefined;
  }

  let dateTimeParts: DateTimeValueParts = splitDateTimeValue(value);
  let timeValue: TimeValueParts | undefined = parseTimeParts(dateTimeParts.timePart);
  if (timeValue === undefined) {
    return undefined;
  }

  let now: Date = new Date();
  return new Date(
    now.getFullYear(),
    now.getMonth(),
    now.getDate(),
    timeValue.hour,
    timeValue.minute
  );
}

function hasExplicitTimeValue(value: string | undefined): boolean {
  if (value === undefined || value.length === 0) {
    return false;
  }
  let dateTimeParts: DateTimeValueParts = splitDateTimeValue(value);
  return parseTimeParts(dateTimeParts.timePart) !== undefined;
}

function hasValidDateValue(value: string): boolean {
  return parseDateValue(value) !== undefined;
}

function hasValidTimeValue(value: string): boolean {
  return parseTimeValue(value) !== undefined;
}

function hasValidDateTimeValue(value: string): boolean {
  let dateTimeParts: DateTimeValueParts = splitDateTimeValue(value);
  if (dateTimeParts.datePart.length === 0 || dateTimeParts.timePart.length === 0) {
    return false;
  }
  return parseDateValue(dateTimeParts.datePart) !== undefined &&
    parseTimeParts(dateTimeParts.timePart) !== undefined;
}

function hasValidDisplayValue(options: DateTimeInputOptions, value: string): boolean {
  if (value.length === 0) {
    return false;
  }
  if (options.enableDate === true && options.enableTime === true) {
    return hasValidDateTimeValue(value);
  }
  if (options.enableDate === true) {
    return hasValidDateValue(value);
  }
  if (options.enableTime === true) {
    return hasValidTimeValue(value);
  }
  return false;
}

function parseFlexibleDateValue(value: string | undefined): Date | undefined {
  if (value === undefined || value.length === 0) {
    return undefined;
  }

  let dateTimeParts: DateTimeValueParts = splitDateTimeValue(value);
  let separator: string = dateTimeParts.datePart.indexOf('/') >= 0 ? '/' : '-';
  let dateParts: string[] = dateTimeParts.datePart.split(separator);
  if (dateParts.length < 3) {
    return undefined;
  }

  let year: number = Number(dateParts[0]);
  let month: number = Number(dateParts[1]);
  let day: number = Number(dateParts[2]);
  if (!isValidDatePart(year, month, day)) {
    return undefined;
  }

  let hour: number = 0;
  let minute: number = 0;
  let timeValue: TimeValueParts | undefined = parseTimeParts(dateTimeParts.timePart);
  if (timeValue !== undefined) {
    hour = timeValue.hour;
    minute = timeValue.minute;
  }
  return new Date(year, month - 1, day, hour, minute);
}

function normalizeDisplayValue(options: DateTimeInputOptions, value: string): string | undefined {
  if (options.enableDate === true && options.enableTime === true) {
    let dateTimeValue: Date | undefined = parseFlexibleDateValue(value);
    let dateTimeParts: DateTimeValueParts = splitDateTimeValue(value);
    if (dateTimeValue === undefined || parseTimeParts(dateTimeParts.timePart) === undefined) {
      return undefined;
    }
    return formatDateTimeValue(dateTimeValue, dateTimeValue);
  }

  if (options.enableDate === true) {
    let dateValue: Date | undefined = parseFlexibleDateValue(value);
    return dateValue === undefined ? undefined : formatDateFromDate(dateValue);
  }

  if (options.enableTime === true) {
    let timeValue: Date | undefined = parseTimeValue(value);
    return timeValue === undefined ? undefined : formatTimeFromDate(timeValue);
  }

  return undefined;
}

function parseBoundaryValue(options: DateTimeInputOptions, value: string | undefined): Date | undefined {
  if (options.enableDate === true) {
    return parseDateValue(value);
  }
  return parseTimeValue(value);
}

function dropInvalidBoundaryValue(
  options: DateTimeInputOptions,
  key: string,
  warningPathPrefix: string = 'customProps'
): void {
  let rawValue: string | undefined = key === 'min' ? options.min : options.max;
  if (rawValue === undefined || rawValue.length === 0) {
    return;
  }
  if (parseBoundaryValue(options, rawValue) !== undefined) {
    return;
  }

  recordDateTimeInputWarning(
    SchemaErrorCode.INVALID_VALUE,
    `Property ${key} expects ISO 8601 compatible value, field has been ignored`,
    resolveDateTimeInputWarningPath(warningPathPrefix, key)
  );
  if (key === 'min') {
    options.min = '';
  } else {
    options.max = '';
  }
}

export function normalizeDateTimeInputOptionsForSchemaWarning(
  sourceOptions: DateTimeInputOptions,
  warningPathPrefix: string = 'customProps'
): DateTimeInputOptions {
  let options: DateTimeInputOptions = {
    value: sourceOptions.value ?? '',
    valuePath: sourceOptions.valuePath,
    enableDate: sourceOptions.enableDate ?? false,
    enableTime: sourceOptions.enableTime ?? false,
    min: sourceOptions.min ?? '',
    max: sourceOptions.max ?? '',
    label: sourceOptions.label ?? ''
  };

  if (options.enableDate !== true && options.enableTime !== true) {
    recordDateTimeInputWarning(
      SchemaErrorCode.INVALID_VALUE,
      'Properties enableDate and enableTime cannot both be false, DateTimeInput remains disabled',
      resolveDateTimeInputWarningPath(warningPathPrefix, 'enableDate')
    );
  }

  dropInvalidBoundaryValue(options, 'min', warningPathPrefix);
  dropInvalidBoundaryValue(options, 'max', warningPathPrefix);

  let minDate: Date | undefined = parseBoundaryValue(options, options.min);
  let maxDate: Date | undefined = parseBoundaryValue(options, options.max);
  if (minDate !== undefined && maxDate !== undefined && minDate.getTime() > maxDate.getTime()) {
    recordDateTimeInputWarning(
      SchemaErrorCode.INVALID_VALUE,
      'Property min must not be later than max, min and max have been ignored',
      resolveDateTimeInputWarningPath(warningPathPrefix, 'min')
    );
    options.min = '';
    options.max = '';
  }

  let sourceValue: string = options.value ?? '';
  if (sourceValue.length > 0 && !hasValidDisplayValue(options, sourceValue)) {
    let normalizedValue: string | undefined = normalizeDisplayValue(options, sourceValue);
    recordDateTimeInputWarning(
      SchemaErrorCode.INVALID_VALUE,
      normalizedValue === undefined ?
        'Property value expects ISO 8601 compatible value, fallback to default value' :
        'Property value expects ISO 8601 compatible value, value has been normalized',
      resolveDateTimeInputWarningPath(warningPathPrefix, 'value')
    );
    options.value = normalizedValue ?? '';
  }

  return options;
}

function formatDateFromDate(value: Date): string {
  const yearStr: string = value.getFullYear().toString();
  const monthStr: string = (value.getMonth() + 1).toString().padStart(2, '0');
  const dayStr: string = value.getDate().toString().padStart(2, '0');
  return `${yearStr}-${monthStr}-${dayStr}`;
}

function formatTimeFromDate(value: Date): string {
  const hourStr: string = value.getHours().toString().padStart(2, '0');
  const minuteStr: string = value.getMinutes().toString().padStart(2, '0');
  return `${hourStr}:${minuteStr}`;
}

function formatDateTimeValue(dateValue: Date, timeValue: Date): string {
  return `${formatDateFromDate(dateValue)} ${formatTimeFromDate(timeValue)}`;
}

function createDateWithTime(baseDate: Date, hour: number, minute: number): Date {
  return new Date(
    baseDate.getFullYear(),
    baseDate.getMonth(),
    baseDate.getDate(),
    hour,
    minute
  );
}

function createDayStart(baseDate: Date): Date {
  return createDateWithTime(baseDate, 0, 0);
}

function createDayEnd(baseDate: Date): Date {
  return createDateWithTime(baseDate, 23, 59);
}

function normalizeTimeToDate(timeValue: Date, baseDate: Date): Date {
  return createDateWithTime(baseDate, timeValue.getHours(), timeValue.getMinutes());
}

function isSameDateValue(firstDate: Date, secondDate: Date): boolean {
  return firstDate.getFullYear() === secondDate.getFullYear() &&
    firstDate.getMonth() === secondDate.getMonth() &&
    firstDate.getDate() === secondDate.getDate();
}

function resolveMinDateOrDefault(options: DateTimeInputOptions): Date {
  return parseDateValue(options.min) ?? new Date(1970, 0, 1);
}

function resolveMaxDateOrDefault(options: DateTimeInputOptions): Date {
  return parseDateValue(options.max) ?? new Date(2100, 11, 31, 23, 59);
}

function resolveDateRangeOrDefault(options: DateTimeInputOptions): DateTimeInputDateRange {
  let start: Date = resolveMinDateOrDefault(options);
  let end: Date = resolveMaxDateOrDefault(options);
  if (start.getTime() <= end.getTime()) {
    return {
      start: start,
      end: end
    };
  }
  return {
    start: end,
    end: start
  };
}

function clampDateValue (value: Date, start: Date, end: Date): Date {
  let clampedTime: number = value.getTime();
  if (clampedTime < start.getTime()) {
    clampedTime = start.getTime();
  }
  if (clampedTime > end.getTime()) {
    clampedTime = end.getTime();
  }
  return new Date(clampedTime);
}

function orderDateRange(start: Date, end: Date): DateTimeInputDateRange {
  if (start.getTime() <= end.getTime()) {
    return {
      start: start,
      end: end
    };
  }
  return {
    start: end,
    end: start
  };
}

function resolveTimeBoundaryOrDefault(
  value: string | undefined,
  baseDate: Date,
  fallbackHour: number,
  fallbackMinute: number
): Date {
  let parsedTime: Date | undefined = parseTimeValue(value);
  if (parsedTime === undefined) {
    return createDateWithTime(baseDate, fallbackHour, fallbackMinute);
  }
  return normalizeTimeToDate(parsedTime, baseDate);
}

function resolveTimeOnlyRangeOrDefault(options: DateTimeInputOptions, baseDate: Date): DateTimeInputDateRange {
  let start: Date = resolveTimeBoundaryOrDefault(options.min, baseDate, 0, 0);
  let end: Date = resolveTimeBoundaryOrDefault(options.max, baseDate, 23, 59);
  return orderDateRange(start, end);
}

function resolveDateTimeTimeRangeOrDefault(options: DateTimeInputOptions, selectedDate: Date): DateTimeInputDateRange {
  let dateRange: DateTimeInputDateRange = resolveDateRangeOrDefault(options);
  let start: Date = createDayStart(selectedDate);
  let end: Date = createDayEnd(selectedDate);

  if (isSameDateValue(selectedDate, dateRange.start)) {
    start = normalizeTimeToDate(dateRange.start, selectedDate);
  }
  if (isSameDateValue(selectedDate, dateRange.end)) {
    end = hasExplicitTimeValue(options.max) ?
      normalizeTimeToDate(dateRange.end, selectedDate) :
      createDayEnd(selectedDate);
  }
  return orderDateRange(start, end);
}

function resolveTimeRangeOrDefault(options: DateTimeInputOptions, baseDate: Date): DateTimeInputDateRange {
  if (options.enableDate === true) {
    return resolveDateTimeTimeRangeOrDefault(options, baseDate);
  }
  return resolveTimeOnlyRangeOrDefault(options, baseDate);
}

function createTimeDate(value: TimePickerResult, baseDate: Date): Date {
  const hour: number = value.hour !== undefined ? value.hour : 0;
  const minute: number = value.minute !== undefined ? value.minute : 0;
  return new Date(
    baseDate.getFullYear(),
    baseDate.getMonth(),
    baseDate.getDate(),
    hour,
    minute
  );
}

export function resolveDateTimeInputConfirmButtonBackgroundColor(
  hasSelectedTextColor: boolean,
  selectedTextColor: ResourceColor
): ResourceColor {
  if (hasSelectedTextColor) {
    return selectedTextColor;
  }
  return DATE_TIME_INPUT_BUTTON_CONFIRM_BACKGROUND_RESOURCE;
}

export function resolveDateTimeInputDialogBackgroundColorByThemeMode(colorMode: ThemeMode | undefined): Resource {
  return colorMode === ThemeMode.DARK ? DATE_TIME_INPUT_DARK_BACKGROUND_COLOR_RESOURCE :
    DATE_TIME_INPUT_BACKGROUND_COLOR_RESOURCE;
}

@Component
struct DateTimeInputSelectionDialog {
  options: DateTimeInputOptions = createDefaultDateTimeInputOptions();
  initialValue: string = '';
  renderId?: number;
  surfaceId?: string;
  componentId?: string;
  @Prop colorMode: ThemeMode | undefined = ThemeMode.LIGHT;
  @Prop hasSelectedTextColor: boolean = false;
  @Prop selectedTextColor: ResourceColor = DATE_TIME_INPUT_BUTTON_CONFIRM_BACKGROUND_RESOURCE;
  onCancelSelection: () => void = () => {
  };
  onConfirmSelection: (value: string) => void = (_value: string) => {
  };
  @State selectedDate: Date = new Date();
  @State selectedTime: Date = new Date();
  @State validationMessage: string = '';
  @State isDateStep: boolean = true;

  aboutToAppear(): void {
    const dateRange: DateTimeInputDateRange = resolveDateRangeOrDefault(this.options);
    const fallbackDate: Date = clampDateValue(new Date(), dateRange.start, dateRange.end);
    const initialDate: Date = clampDateValue(
      parseDateValue(this.initialValue) ?? fallbackDate,
      dateRange.start,
      dateRange.end
    );
    const initialTime: Date = parseTimeValue(this.initialValue) ?? initialDate;
    const initialTimeBaseDate: Date = this.shouldRenderDate() ? initialDate : initialTime;
    const initialTimeRange: DateTimeInputDateRange = resolveTimeRangeOrDefault(this.options, initialTimeBaseDate);

    this.selectedDate = initialDate;
    this.selectedTime = clampDateValue(
      normalizeTimeToDate(initialTime, initialTimeBaseDate),
      initialTimeRange.start,
      initialTimeRange.end
    );
    this.validationMessage = '';
  }

  build() {
    Column({ space: DATE_TIME_INPUT_SECTION_SPACE_RESOURCE }) {
      Text(this.resolveDialogTitle())
        .fontSize(DATE_TIME_INPUT_DIALOG_TITLE_FONT_SIZE_RESOURCE)
        .fontWeight(FontWeight.Bold)
        .width('100%')
        .textAlign(TextAlign.Center)
        .maxLines(1)
        .padding({
          left: DATE_TIME_INPUT_DIALOG_TITLE_PADDING_RESOURCE,
          right: DATE_TIME_INPUT_DIALOG_TITLE_PADDING_RESOURCE
        });

      if (this.shouldShowDatePicker()) {
        this.DatePickerContent();
      }

      if (this.shouldShowTimePicker()) {
        this.TimePickerContent();
      }

      if (this.validationMessage.length > 0) {
        Text(this.validationMessage)
          .width('100%')
          .fontSize(DATE_TIME_INPUT_ERROR_FONT_SIZE_RESOURCE)
          .fontColor(DATE_TIME_INPUT_ERROR_COLOR_RESOURCE);
      }

      Row({ space: DATE_TIME_INPUT_DIALOG_BUTTON_SPACE_RESOURCE }) {
        Button(DATE_TIME_INPUT_CANCEL_TEXT_RESOURCE)
          .layoutWeight(1)
          .backgroundColor(DATE_TIME_INPUT_BUTTON_CANCEL_BACKGROUND_RESOURCE)
          .fontColor(DATE_TIME_INPUT_TEXT_PRIMARY_COLOR_RESOURCE)
          .onClick(() => {
            this.onCancelSelection();
          });

        if (this.isCombinedMode() && this.isDateStep) {
          Button(DATE_TIME_INPUT_CONFIRM_TEXT_RESOURCE)
            .layoutWeight(1)
            .backgroundColor(resolveDateTimeInputConfirmButtonBackgroundColor(
              this.hasSelectedTextColor,
              this.selectedTextColor
            ))
            .fontColor(DATE_TIME_INPUT_TEXT_INVERSE_COLOR_RESOURCE)
            .onClick(() => {
              this.isDateStep = false;
            });
        } else {
          Button(DATE_TIME_INPUT_CONFIRM_TEXT_RESOURCE)
            .layoutWeight(1)
            .backgroundColor(resolveDateTimeInputConfirmButtonBackgroundColor(
              this.hasSelectedTextColor,
              this.selectedTextColor
            ))
            .fontColor(DATE_TIME_INPUT_TEXT_INVERSE_COLOR_RESOURCE)
            .onClick(() => {
              let confirmedValue: string = this.resolveConfirmedValue();
              let validationResult: DateTimeInputValidationResult = validateDateTimeInputChecks(
                this.renderId,
                this.surfaceId,
                this.componentId,
                confirmedValue
              );
              if (!validationResult.valid) {
                this.validationMessage = validationResult.message;
                return;
              }
              this.validationMessage = '';
              this.onConfirmSelection(confirmedValue);
            });
        }
      }
      .width('100%');
    }
    .width('90%')
    .padding(DATE_TIME_INPUT_DIALOG_PADDING_RESOURCE)
    .backgroundColor(resolveDateTimeInputDialogBackgroundColorByThemeMode(this.colorMode))
    .borderRadius(DATE_TIME_INPUT_DIALOG_RADIUS_RESOURCE);
  }

  @Builder
  private DatePickerContent() {
    if (this.hasSelectedTextColor) {
      DatePicker({
        start: resolveDateRangeOrDefault(this.options).start,
        end: resolveDateRangeOrDefault(this.options).end,
        selected: this.selectedDate
      })
        .selectedTextStyle({ color: this.selectedTextColor })
        .width('100%')
        .constraintSize({ minHeight: DATE_TIME_INPUT_PICKER_MIN_HEIGHT })
        .onDateChange((value: Date) => {
          this.handleDateChange(value);
        });
    } else {
      DatePicker({
        start: resolveDateRangeOrDefault(this.options).start,
        end: resolveDateRangeOrDefault(this.options).end,
        selected: this.selectedDate
      })
        .width('100%')
        .constraintSize({ minHeight: DATE_TIME_INPUT_PICKER_MIN_HEIGHT })
        .onDateChange((value: Date) => {
          this.handleDateChange(value);
        });
    }
  }

  @Builder
  private TimePickerContent() {
    if (this.hasSelectedTextColor) {
      TimePicker({
        selected: this.resolveTimePickerBoundary().selected,
        start: this.resolveTimePickerBoundary().start,
        end: this.resolveTimePickerBoundary().end
      })
        .selectedTextStyle({ color: this.selectedTextColor })
        .width('100%')
        .constraintSize({ minHeight: DATE_TIME_INPUT_PICKER_MIN_HEIGHT })
        .useMilitaryTime(true)
        .onChange((value: TimePickerResult) => {
          this.handleTimeChange(value);
        });
    } else {
      TimePicker({
        selected: this.resolveTimePickerBoundary().selected,
        start: this.resolveTimePickerBoundary().start,
        end: this.resolveTimePickerBoundary().end
      })
        .width('100%')
        .constraintSize({ minHeight: DATE_TIME_INPUT_PICKER_MIN_HEIGHT })
        .useMilitaryTime(true)
        .onChange((value: TimePickerResult) => {
          this.handleTimeChange(value);
        });
    }
  }

  private handleDateChange(value: Date): void {
    let dateRange: DateTimeInputDateRange = resolveDateRangeOrDefault(this.options);
    let nextDate: Date = clampDateValue(
      value,
      dateRange.start,
      dateRange.end
    );
    this.selectedDate = nextDate;
    this.selectedTime = this.resolveBoundedSelectedTime(nextDate);
    this.validationMessage = '';
  }

  private handleTimeChange(value: TimePickerResult): void {
    let timeBaseDate: Date = this.resolveTimeBaseDate();
    this.selectedTime = this.resolveBoundedSelectedTime(
      timeBaseDate,
      createTimeDate(value, timeBaseDate)
    );
    this.validationMessage = '';
  }

  private shouldRenderDate(): boolean {
    return this.options.enableDate === true;
  }

  private shouldRenderTime(): boolean {
    return this.options.enableTime === true;
  }

  private isCombinedMode(): boolean {
    return this.options.enableDate === true && this.options.enableTime === true;
  }

  private shouldShowDatePicker(): boolean {
    if (this.isCombinedMode()) {
      return this.isDateStep;
    }
    return this.options.enableDate === true;
  }

  private shouldShowTimePicker(): boolean {
    if (this.isCombinedMode()) {
      return !this.isDateStep;
    }
    return this.options.enableTime === true;
  }

  private resolveDialogTitle(): ResourceStr {
    if (this.isCombinedMode()) {
      return this.isDateStep ? DATE_TIME_INPUT_DIALOG_TITLE_DATE_RESOURCE :
        DATE_TIME_INPUT_DIALOG_TITLE_TIME_RESOURCE;
    }
    if (this.options.enableDate === true) {
      return DATE_TIME_INPUT_DIALOG_TITLE_DATE_RESOURCE;
    }
    if (this.options.enableTime === true) {
      return DATE_TIME_INPUT_DIALOG_TITLE_TIME_RESOURCE;
    }
    return DATE_TIME_INPUT_DIALOG_TITLE_DEFAULT_RESOURCE;
  }

  private resolveConfirmedValue(): string {
    if (this.shouldRenderDate() && this.shouldRenderTime()) {
      return formatDateTimeValue(this.resolveBoundedSelectedDate(), this.resolveBoundedSelectedTime());
    }
    if (this.options.enableDate === true) {
      return formatDateFromDate(this.resolveBoundedSelectedDate());
    }
    if (this.options.enableTime === true) {
      return formatTimeFromDate(this.resolveBoundedSelectedTime());
    }
    return '';
  }

  private resolveBoundedSelectedDate(): Date {
    let dateRange: DateTimeInputDateRange = resolveDateRangeOrDefault(this.options);
    return clampDateValue(this.selectedDate, dateRange.start, dateRange.end);
  }

  private resolveTimeBaseDate(baseDate?: Date): Date {
    if (baseDate !== undefined) {
      return baseDate;
    }
    if (this.shouldRenderDate()) {
      return this.resolveBoundedSelectedDate();
    }
    return this.selectedTime;
  }

  private resolveTimePickerBoundary(): TimePickerBoundary {
    let timeBaseDate: Date = this.resolveTimeBaseDate();
    let timeRange: DateTimeInputDateRange = resolveTimeRangeOrDefault(this.options, timeBaseDate);
    let selected: Date = clampDateValue(
      normalizeTimeToDate(this.selectedTime, timeBaseDate),
      timeRange.start,
      timeRange.end
    );
    return {
      selected: selected,
      start: timeRange.start,
      end: timeRange.end
    };
  }

  private resolveBoundedSelectedTime(baseDate?: Date, timeValue?: Date): Date {
    let timeBaseDate: Date = this.resolveTimeBaseDate(baseDate);
    let timeRange: DateTimeInputDateRange = resolveTimeRangeOrDefault(this.options, timeBaseDate);
    return clampDateValue(
      normalizeTimeToDate(timeValue ?? this.selectedTime, timeBaseDate),
      timeRange.start,
      timeRange.end
    );
  }
}

@Component
export struct CustomDateTimeInput {
  @Prop @Watch('onAttributeChanged') attribute: CustomComponentAttribute | undefined = undefined;
  @State displayValue: string = '';
  @State displayValueSource: string = '';
  private dialogContent: ComponentContent<DateTimeInputDialogAttribute> | null = null;
  private dialogAttribute: DateTimeInputDialogAttribute | null = null;
  private dialogUIContext: UIContext | null = null;
  private resolvedOptions: DateTimeInputOptions = createDefaultDateTimeInputOptions();
  private hasResolvedOptions: boolean = false;
  private lastResolvedSignature: string = '';

  private onAttributeChanged(): void {
    this.ensureResolvedOptionsSynchronized();
    this.refreshDisplayValue(this.resolvedOptions);
    this.updatePickerDialogTheme();
  }

  aboutToAppear(): void {
    this.ensureResolvedOptionsSynchronized();
    this.refreshDisplayValue(this.resolvedOptions);
  }

  aboutToDisappear(): void {
    this.dismissPickerDialog();
  }

  build() {
    this.DateTimeInputRoot(this.getResolvedOptions());
  }

  @Builder
  private DateTimeInputRoot(options: DateTimeInputOptions) {
    Column({ space: DATE_TIME_INPUT_CONTAINER_SPACE_RESOURCE }) {
      this.DateTimeInputContent(options);
    }
    .width('100%')
    .layoutWeight(this.resolveWeight())
    .margin(this.resolveMargin())
    .accessibilityGroup(true)
    .accessibilityText(this.resolveAccessibilityLabel())
    .accessibilityDescription(this.resolveAccessibilityDescription())
    .id(this.attribute?.id ?? '');
  }

  @Builder
  private DateTimeInputContent(options: DateTimeInputOptions) {
    Row() {
      Text(this.resolveInputText(options))
        .fontSize(DATE_TIME_INPUT_TEXT_FONT_SIZE_RESOURCE)
        .maxLines(1)
        .fontColor(this.resolveInputTextColor(options))
        .layoutWeight(1)
        .textOverflow({ overflow: TextOverflow.Ellipsis });
    }
    .width('100%')
    .padding({
      left: DATE_TIME_INPUT_HORIZONTAL_PADDING_RESOURCE,
      right: DATE_TIME_INPUT_HORIZONTAL_PADDING_RESOURCE,
      top: DATE_TIME_INPUT_VERTICAL_PADDING_RESOURCE,
      bottom: DATE_TIME_INPUT_VERTICAL_PADDING_RESOURCE
    })
    .borderRadius(DATE_TIME_INPUT_RADIUS_RESOURCE)
    .border({
      width: DATE_TIME_INPUT_BORDER_WIDTH_RESOURCE,
      color: this.canOpenDialog(options) ? DATE_TIME_INPUT_BORDER_ENABLED_COLOR_RESOURCE :
        DATE_TIME_INPUT_BORDER_DISABLED_COLOR_RESOURCE
    })
    .backgroundColor(this.resolveInputBackgroundColor())
    .onClick(() => {
      this.openPickerDialog(options);
    });
  }

  private resolveInputTextColor(options: DateTimeInputOptions): Resource {
    if (!this.hasDisplayValue(options)) {
      return DATE_TIME_INPUT_PLACEHOLDER_COLOR_RESOURCE;
    }
    return this.attribute?.componentTheme?.colorMode === ThemeMode.DARK ?
      DATE_TIME_INPUT_TEXT_INVERSE_COLOR_RESOURCE :
      DATE_TIME_INPUT_TEXT_PRIMARY_COLOR_RESOURCE;
  }

  private resolveInputBackgroundColor(): Resource {
    return resolveDateTimeInputDialogBackgroundColorByThemeMode(this.attribute?.componentTheme?.colorMode);
  }

  private openPickerDialog(options?: DateTimeInputOptions): void {
    let resolvedOptions: DateTimeInputOptions = options ?? this.getResolvedOptions();
    if (resolvedOptions.enableDate !== true && resolvedOptions.enableTime !== true) {
      return;
    }

    let uiContext: UIContext | undefined = resolveDialogUIContext('CustomDateTimeInput', () => this.getUIContext());
    if (uiContext === undefined) {
      console.error('CustomDateTimeInput openPickerDialog failed: host UIContext is unavailable');
      return;
    }

    try {
      this.dismissPickerDialog();
      let dialogAttribute: DateTimeInputDialogAttribute = this.createPickerDialogAttribute(resolvedOptions);
      let dialogContent = new ComponentContent<DateTimeInputDialogAttribute>(
        uiContext,
        wrapBuilder(dateTimeInputSelectionDialogBuilder),
        dialogAttribute
      );
      this.dialogUIContext = uiContext;
      this.dialogAttribute = dialogAttribute;
      this.dialogContent = dialogContent;
      showCustomDialogSafely(
        'CustomDateTimeInput openPickerDialog',
        uiContext,
        dialogContent,
        () => this.onPickerDialogDidDisappear(dialogContent)
      );
    } catch (error) {
      console.error(`CustomDateTimeInput openPickerDialog failed: ${JSON.stringify(error)}`);
      this.clearPickerDialog();
    }
  }

  private createPickerDialogAttribute(resolvedOptions: DateTimeInputOptions): DateTimeInputDialogAttribute {
    let brandColor: string | undefined = resolveComponentBrandColor(this.attribute);
    return {
      options: resolvedOptions,
      initialValue: this.resolveDisplayValue(resolvedOptions),
      renderId: this.attribute?.renderId,
      surfaceId: this.attribute?.surfaceId,
      componentId: this.attribute?.id,
      colorMode: this.attribute?.componentTheme?.colorMode,
      hasSelectedTextColor: brandColor !== undefined,
      selectedTextColor: brandColor ?? DATE_TIME_INPUT_BUTTON_CONFIRM_BACKGROUND_RESOURCE,
      onCancelSelection: () => {
        this.dismissPickerDialog();
      },
      onConfirmSelection: (value: string) => {
        let sourceValue: string = resolvedOptions.value ?? '';
        let syncSuccess: boolean = updateDateTimeInputDataModelValue(
          this.attribute?.renderId,
          this.attribute?.surfaceId,
          this.attribute?.id,
          value
        );
        if (resolvedOptions.valuePath !== undefined && resolvedOptions.valuePath.length > 0 && !syncSuccess) {
          console.warn('CustomDateTimeInput confirm skipped: bound dataModel sync failed');
          return;
        }
        this.displayValue = hasValidDisplayValue(resolvedOptions, value) ? value : '';
        this.displayValueSource = sourceValue;
        this.dismissPickerDialog();
      }
    };
  }

  private updatePickerDialogTheme(): void {
    let dialogContent: ComponentContent<DateTimeInputDialogAttribute> | null = this.dialogContent;
    let currentAttribute: DateTimeInputDialogAttribute | null = this.dialogAttribute;
    if (dialogContent === null || currentAttribute === null) {
      return;
    }

    let brandColor: string | undefined = resolveComponentBrandColor(this.attribute);
    let colorMode: ThemeMode | undefined = this.attribute?.componentTheme?.colorMode;
    let hasSelectedTextColor: boolean = brandColor !== undefined;
    let selectedTextColor: ResourceColor = brandColor ?? DATE_TIME_INPUT_BUTTON_CONFIRM_BACKGROUND_RESOURCE;
    if (currentAttribute.colorMode === colorMode &&
      currentAttribute.hasSelectedTextColor === hasSelectedTextColor &&
      currentAttribute.selectedTextColor === selectedTextColor) {
      return;
    }

    let updatedAttribute: DateTimeInputDialogAttribute = {
      options: currentAttribute.options,
      initialValue: currentAttribute.initialValue,
      renderId: currentAttribute.renderId,
      surfaceId: currentAttribute.surfaceId,
      componentId: currentAttribute.componentId,
      colorMode: colorMode,
      hasSelectedTextColor: hasSelectedTextColor,
      selectedTextColor: selectedTextColor,
      onCancelSelection: currentAttribute.onCancelSelection,
      onConfirmSelection: currentAttribute.onConfirmSelection
    };
    this.dialogAttribute = updatedAttribute;
    dialogContent.update(updatedAttribute);
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

  private getResolvedOptions(): DateTimeInputOptions {
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
    this.resolvedOptions = this.resolveOptions();
    this.hasResolvedOptions = true;
    this.lastResolvedSignature = buildCustomPropsSignature(this.attribute);
  }

  private dismissPickerDialog(): void {
    let dialogContent: ComponentContent<DateTimeInputDialogAttribute> | null = this.dialogContent;
    let uiContext: UIContext | null = this.dialogUIContext;
    this.clearPickerDialog();
    closeCustomDialogSafely('CustomDateTimeInput dismissPickerDialog', uiContext, dialogContent);
  }

  private clearPickerDialog(): void {
    this.dialogContent = null;
    this.dialogAttribute = null;
    this.dialogUIContext = null;
  }

  private onPickerDialogDidDisappear(dialogContent: ComponentContent<DateTimeInputDialogAttribute> | null): void {
    if (dialogContent !== null && this.dialogContent === dialogContent) {
      this.clearPickerDialog();
    }
  }

  private canOpenDialog(options?: DateTimeInputOptions): boolean {
    let resolvedOptions: DateTimeInputOptions = options ?? this.getResolvedOptions();
    return resolvedOptions.enableDate === true || resolvedOptions.enableTime === true;
  }

  private hasDisplayValue(options?: DateTimeInputOptions): boolean {
    let resolvedOptions: DateTimeInputOptions = options ?? this.getResolvedOptions();
    return this.resolveDisplayValue(resolvedOptions).length > 0;
  }

  private resolveInputText(options?: DateTimeInputOptions): ResourceStr {
    let resolvedOptions: DateTimeInputOptions = options ?? this.getResolvedOptions();
    let displayValue: string = this.resolveDisplayValue(resolvedOptions);
    if (displayValue.length > 0) {
      return displayValue;
    }
    return this.resolvePlaceholder(resolvedOptions);
  }

  private resolvePlaceholder(options: DateTimeInputOptions): ResourceStr {
    if (options.label !== undefined && options.label.length > 0) {
      return options.label;
    }
    if (options.enableDate === true && options.enableTime === true) {
      return DATE_TIME_INPUT_PLACEHOLDER_DATE_TIME_RESOURCE;
    }
    if (options.enableDate === true) {
      return DATE_TIME_INPUT_PLACEHOLDER_DATE_RESOURCE;
    }
    if (options.enableTime === true) {
      return DATE_TIME_INPUT_PLACEHOLDER_TIME_RESOURCE;
    }
    return DATE_TIME_INPUT_PLACEHOLDER_DISABLED_RESOURCE;
  }

  private resolveDisplayValue(options: DateTimeInputOptions): string {
    let sourceValue: string = options.value ?? '';
    if (this.displayValueSource === sourceValue) {
      return this.displayValue;
    }
    return hasValidDisplayValue(options, sourceValue) ? sourceValue : '';
  }

  private refreshDisplayValue(options: DateTimeInputOptions): void {
    let sourceValue: string = options.value ?? '';
    if (sourceValue === this.displayValueSource && hasValidDisplayValue(options, this.displayValue)) {
      return;
    }
    this.displayValue = hasValidDisplayValue(options, sourceValue) ? sourceValue : '';
    this.displayValueSource = sourceValue;
  }

  private resolveOptions(): DateTimeInputOptions {
    return withAttributeSchemaWarningContext(this.attribute, (): DateTimeInputOptions => {
      let parsed = parseCustomProps(this.attribute, 'CustomDateTimeInput');
      if (parsed === undefined) {
        return createDefaultDateTimeInputOptions();
      }

      const warningPathPrefix: string = resolveDateTimeInputWarningPathPrefix(this.attribute);
      SchemaLocalPropertyHelper.recordUnknownFields(
        parsed,
        ['label', 'value', 'enableDate', 'enableTime', 'min', 'max', DATE_TIME_INPUT_BINDINGS_KEY],
        'DateTimeInput',
        warningPathPrefix
      );
      let resolvedOptions: DateTimeInputOptions = createDefaultDateTimeInputOptions();

      let labelValue: string | undefined =
        resolveDateTimeInputDynamicStringProperty(parsed, 'label', this.attribute, false, warningPathPrefix);
      let labelPath: string | undefined = resolveCustomBindingPath(parsed, 'label');
      if (labelValue === undefined) {
        labelValue = resolveDynamicStringPathOptionValue(labelPath, this.attribute);
      }
      if (labelValue !== undefined) {
        resolvedOptions.label = labelValue;
      }

      let valuePath: string | undefined = resolveCustomBindingPath(parsed, 'value');
      let valueRequired: boolean = valuePath === undefined;
      let valueValue: string | undefined =
        resolveDateTimeInputDynamicStringProperty(parsed, 'value', this.attribute, valueRequired, warningPathPrefix);
      if (valueValue === undefined) {
        valueValue = resolveDynamicStringPathOptionValue(valuePath, this.attribute);
      }
      if (valueValue !== undefined) {
        resolvedOptions.value = valueValue;
      } else {
        console.error(`CustomDateTimeInput value is null`);
      }
      if (valuePath !== undefined) {
        resolvedOptions.valuePath = valuePath;
      }

      resolvedOptions.enableDate = SchemaLocalPropertyHelper.normalizeBooleanProperty(
        parsed,
        'enableDate',
        false,
        'DateTimeInput',
        warningPathPrefix
      );
      resolvedOptions.enableTime = SchemaLocalPropertyHelper.normalizeBooleanProperty(
        parsed,
        'enableTime',
        false,
        'DateTimeInput',
        warningPathPrefix
      );
      resolvedOptions.min = SchemaLocalPropertyHelper.normalizePlainStringProperty(
        parsed,
        'min',
        '',
        'DateTimeInput',
        warningPathPrefix
      );
      resolvedOptions.max = SchemaLocalPropertyHelper.normalizePlainStringProperty(
        parsed,
        'max',
        '',
        'DateTimeInput',
        warningPathPrefix
      );

      return normalizeDateTimeInputOptionsForSchemaWarning(resolvedOptions, warningPathPrefix);
    });
  }
}

@Builder
function dateTimeInputSelectionDialogBuilder(attribute: DateTimeInputDialogAttribute): void {
  DateTimeInputSelectionDialog({
    options: attribute.options,
    initialValue: attribute.initialValue,
    renderId: attribute.renderId,
    surfaceId: attribute.surfaceId,
    componentId: attribute.componentId,
    colorMode: attribute.colorMode,
    hasSelectedTextColor: attribute.hasSelectedTextColor ?? false,
    selectedTextColor: attribute.selectedTextColor ?? DATE_TIME_INPUT_BUTTON_CONFIRM_BACKGROUND_RESOURCE,
    onCancelSelection: attribute.onCancelSelection,
    onConfirmSelection: attribute.onConfirmSelection
  });
}

function validateDateTimeInputChecks(
  renderId: number | undefined,
  surfaceId: string | undefined,
  componentId: string | undefined,
  value: string
): DateTimeInputValidationResult {
  if (renderId === undefined || surfaceId === undefined || surfaceId.length === 0 ||
      componentId === undefined || componentId.length === 0) {
    return { valid: true, message: '' };
  }
  try {
    let result: ValidateCustomComponentChecksResult | undefined = nativeEngine.validateCustomComponentChecks(
      renderId,
      surfaceId,
      componentId,
      JSON.stringify(value)
    );
    if (result === undefined) {
      console.warn('CustomDateTimeInput validateChecks returned undefined');
      return { valid: true, message: '' };
    }
    return {
      valid: result.valid,
      message: result.message
    };
  } catch (error) {
    console.error(`CustomDateTimeInput validateChecks failed: ${JSON.stringify(error)}`);
    return { valid: true, message: '' };
  }
}

function updateDateTimeInputDataModelValue(
  renderId: number | undefined,
  surfaceId: string | undefined,
  componentId: string | undefined,
  value: string
): boolean {
  if (renderId === undefined || componentId === undefined || componentId.length === 0) {
    return false;
  }

  try {
    let result: SyncComponentBoundDataModelResult = nativeEngine.syncComponentBoundDataModel(
      renderId,
      surfaceId ?? '',
      componentId,
      'value',
      JSON.stringify(value)
    );
    if (result.success !== true) {
      console.warn(`CustomDateTimeInput syncComponentBoundDataModel failed: ${result.errorMessage ?? ''}`);
      return false;
    }
    return true;
  } catch (error) {
    console.error(`CustomDateTimeInput syncComponentBoundDataModel failed: ${JSON.stringify(error)}`);
    return false;
  }
}

function resolveCustomBindingPath(parsed: Record<string, Object>, field: string): string | undefined {
  let bindingsValue: Object | undefined = parsed[DATE_TIME_INPUT_BINDINGS_KEY];
  if (typeof bindingsValue === 'object' && bindingsValue !== null && !Array.isArray(bindingsValue)) {
    let bindings = bindingsValue as Record<string, Object>;
    let pathValue: Object | undefined = bindings[field];
    if (typeof pathValue === 'string' && pathValue.length > 0) {
      return pathValue;
    }
  }
  return resolveDynamicStringPath(parsed[field]);
}

function resolveDynamicStringPath(value: Object | undefined): string | undefined {
  if (typeof value !== 'object' || value === null || Array.isArray(value)) {
    return undefined;
  }
  let valueObject = value as Record<string, Object>;
  let pathValue: Object | undefined = valueObject['path'];
  return typeof pathValue === 'string' && pathValue.length > 0 ? pathValue : undefined;
}

@Builder
export function customDateTimeInputBuilder(attribute: CustomComponentAttribute): void {
  CustomDateTimeInput({ attribute: attribute });
}

export function createDateTimeInputDefinition(): CustomComponentDefinition {
  const schemaProvider: SchemaProvider = (version: string) => {
    return SchemaResourceLoader.loadA2UISchema(version, 'components/DateTimeInput.json');
  };
  return {
    type: DATA_TIME_INPUT_TYPE,
    builder: wrapBuilder(customDateTimeInputBuilder),
    schemaProvider: schemaProvider
  };
}

export function asCatalogItem(): CatalogItem {
  const definition: CustomComponentDefinition = createDateTimeInputDefinition();
  return CatalogItem.forComponent(
    definition.type,
    definition.schemaProvider ?? ((_version: string): string => ''),
    definition.builder
  ).markInnerNative(false);
}
