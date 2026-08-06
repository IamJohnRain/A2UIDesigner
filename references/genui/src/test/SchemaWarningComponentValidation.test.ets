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

import { describe, beforeEach, afterEach, it, xit, expect } from '@ohos/hypium';
import {
  ComponentTheme,
  CustomComponentAttribute
} from '../main/ets/core/components/A2UI/CustomComponent';
import {
  CustomComponentDefinition,
  CustomComponentFactory
} from '../main/ets/core/components/A2UI/CustomComponentFactory';
import {
  DateTimeInputOptions,
  normalizeDateTimeInputOptionsForSchemaWarning
} from '../main/ets/core/components/A2UI/CustomDateTimeInput';
import {
  AudioPlayerOptions,
  isSupportedAudioUrl,
  normalizeAudioPlayerOptionsForSchemaWarning,
  resolveAudioPlayerOptionsForSchemaWarning
} from '../main/ets/core/components/A2UI/CustomAudioPlayer';
import { CatalogImpl } from '../main/ets/core/base/CatalogImpl';
import { CatalogFactoryCore } from '../main/ets/core/base/FactoriesCore';
import { resolveIconRenderState } from '../main/ets/core/components/A2UI/CustomIcon';
import {
  isSupportedVideoUrl,
  resolveVideoOptionsForSchemaWarning
} from '../main/ets/core/components/A2UI/CustomVideo';
import { validateExtendedSelectSchemaForWarning } from '../main/ets/core/components/extended/ExtendedSelect';
import {
  SchemaErrorCode,
  SchemaErrorInfo,
  SchemaErrorInfoManager
} from '../main/ets/core/schema/SchemaErrorInfoManager';
import { ChangeReason } from '../main/ets/core/types/types';
import { DynamicValueResolver, ResolveValueCallback } from '../main/ets/interface/DynamicValueResolver';
import { A2UIValueType } from '../main/ets/interface/Types';
import { SchemaResourceLoader } from '../main/ets/core/schema/SchemaResourceLoader';
import { SchemaValidator } from '../main/ets/core/schema/SchemaValidator';
import { SchemaLocalPropertyHelper } from '../main/ets/core/schema/SchemaLocalPropertyHelper';
import {
  createExtendedRowDefinition,
  resolveExtendedRowPrivateValue
} from '../main/ets/core/components/extended/ExtendedRow';
import {
  buildExtendedTabsTemplateInstanceChildId,
  dispatchExtendedTabsIndexRangeWarning,
  ExtendedTabsIndexRangeResult,
  normalizeExtendedTabsIndexForItemCount,
  normalizeExtendedTabsIndexValue,
  resolveExtendedTabsBorderRadius
} from '../main/ets/core/components/extended/ExtendedTabs';
import {
  normalizeExtendedTabContentFontWeightValue,
  normalizeExtendedTabContentMetricValue
} from '../main/ets/core/components/extended/ExtendedTabContent';
import { CustomComponentDynamicValueResolver } from '../main/ets/core/types/runtime/CustomComponentDynamicValueResolver';
import {
  ExtendedCommonStyleOptions,
  mergeExtendedCommonStyleOptions,
  resolveExtendedCommonStyleOptions
} from '../main/ets/core/components/extended/ExtendedCommonStyleModifier';

@Builder
function schemaWarningPlaceholderBuilder(_attribute: CustomComponentAttribute): void {}

class CommonStyleExpressionResolverStub implements DynamicValueResolver {
  resolvePropertyValue<T>(
    _key: string,
    value: A2UIValueType,
    callback: ResolveValueCallback<T>
  ): void {
    callback(this.evaluateValue<T>(value));
  }

  evaluateValue<T>(value: A2UIValueType): T {
    if (value === '{{ widthExpression }}') {
      return 240 as T;
    }
    if (value === '{{ positiveFlexShrinkExpression }}') {
      return 20 as T;
    }
    if (value === '{{ invalidBackgroundSizeExpression }}') {
      return undefined as T;
    }
    if (value === '{{ wrongBackgroundSizeExpression }}') {
      return 'bogus' as T;
    }
    if (value === '{{ numericColorExpression }}') {
      return 0xFF102030 as T;
    }
    return value as T;
  }
}

function hasWarning(path: string, messagePart: string): boolean {
  let warnings: SchemaErrorInfo[] = SchemaErrorInfoManager.getInstance().getWarnings();
  for (const warning of warnings) {
    if (warning.path === path && warning.message.indexOf(messagePart) >= 0) {
      return true;
    }
  }
  return false;
}

function getComponentSchema(catalog: CatalogImpl, componentName: string): string {
  for (const item of catalog.components) {
    if (item.name === componentName) {
      return item.schemaProvider('v0.9');
    }
  }
  return '';
}

function getCustomComponentSchema(definition: CustomComponentDefinition): string {
  CustomComponentFactory.registerCustomComponent(definition);
  return CustomComponentFactory.createCatalogItem(definition.type).schemaProvider('v0.9');
}

function getExtendedCommonSchema(): string {
  return SchemaResourceLoader.loadSchema('schema/Extended/common_types.json');
}

function createCustomComponentAttribute(
  type: string,
  id: string,
  customProps?: Record<string, Object>,
  resolver?: DynamicValueResolver
): CustomComponentAttribute {
  let componentTheme: ComponentTheme = new Object() as ComponentTheme;
  return {
    type: type,
    id: id,
    surfaceId: 'schemaWarningSurface',
    protocolVersion: 'v0.9',
    catalogId: '',
    componentTheme: componentTheme,
    customProps: customProps,
    changeReason: ChangeReason.UPDATE_COMPONENT,
    resolver: resolver ?? new CustomComponentDynamicValueResolver(),
    builder: wrapBuilder(schemaWarningPlaceholderBuilder)
  };
}

function createBackgroundImageSizeWithStyleProps(widthValue: Object, heightValue: Object): Record<string, Object> {
  let sizeRecord: Record<string, Object> = new Object() as Record<string, Object>;
  sizeRecord['width'] = widthValue;
  sizeRecord['height'] = heightValue;
  let styles: Record<string, Object> = new Object() as Record<string, Object>;
  styles['backgroundImageSizeWithStyle'] = sizeRecord as Object;
  let props: Record<string, Object> = new Object() as Record<string, Object>;
  props['styles'] = styles as Object;
  return props;
}

function createZeroLayoutWeightAndConstraintSizeProps(): Record<string, Object> {
  let constraintSize: Record<string, Object> = new Object() as Record<string, Object>;
  constraintSize['minWidth'] = 0 as Object;
  constraintSize['maxWidth'] = 0 as Object;
  constraintSize['minHeight'] = 0 as Object;
  constraintSize['maxHeight'] = 0 as Object;

  let styles: Record<string, Object> = new Object() as Record<string, Object>;
  styles['layoutWeight'] = 0 as Object;
  styles['constraintSize'] = constraintSize as Object;
  styles['padding'] = 12 as Object;
  styles['borderWidth'] = 2 as Object;

  let props: Record<string, Object> = new Object() as Record<string, Object>;
  props['styles'] = styles as Object;
  return props;
}

function createPathBinding(path: string): Record<string, Object> {
  let binding: Record<string, Object> = new Object() as Record<string, Object>;
  binding['path'] = path as Object;
  return binding;
}

function hasWarningCode(path: string, code: SchemaErrorCode): boolean {
  let warnings: SchemaErrorInfo[] = SchemaErrorInfoManager.getInstance().getWarnings();
  for (const warning of warnings) {
    if (warning.path === path && warning.code === code) {
      return true;
    }
  }
  return false;
}

export default function SchemaWarningComponentValidationUnitTest() {
  describe('SchemaWarningComponentValidationTest', () => {
    beforeEach(() => {
      SchemaErrorInfoManager.getInstance().clear();
    });

    afterEach(() => {
      SchemaErrorInfoManager.getInstance().clear();
    });

    it('should_prefer_extended_row_style_private_fields_over_top_level_fields', 0, () => {
      let styles: Record<string, Object> = {
        'justifyContent': 'center',
        'alignItems': 'bottom',
        'wrap': 'wrap'
      };
      let customProps: Record<string, Object> = {
        'justifyContent': 'end',
        'alignItems': 'top',
        'wrap': 'noWrap',
        'styles': styles
      };

      expect(resolveExtendedRowPrivateValue(customProps, 'justifyContent') as string).assertEqual('center');
      expect(resolveExtendedRowPrivateValue(customProps, 'alignItems') as string).assertEqual('bottom');
      expect(resolveExtendedRowPrivateValue(customProps, 'wrap') as string).assertEqual('wrap');
    });

    it('should_reject_non_integer_or_negative_extended_tabs_index_results', 0, () => {
      expect(normalizeExtendedTabsIndexValue(2, 0)).assertEqual(2);
      expect(normalizeExtendedTabsIndexValue(1.5, 3)).assertEqual(3);
      expect(normalizeExtendedTabsIndexValue(-1, 4)).assertEqual(4);
      expect(normalizeExtendedTabsIndexValue(Number.NaN, 5)).assertEqual(5);
    });

    it('should_apply_radius_only_to_capsule_extended_tabs', 0, () => {
      expect(resolveExtendedTabsBorderRadius('capsule')).assertEqual(18);
      expect(resolveExtendedTabsBorderRadius('underline')).assertEqual(0);
      expect(resolveExtendedTabsBorderRadius('')).assertEqual(0);
    });

    it('should_preserve_absolute_template_path_when_building_extended_tabs_child_id', 0, () => {
      expect(buildExtendedTabsTemplateInstanceChildId('tabTemplate', '/tabs', 0))
        .assertEqual('/tabstabTemplate:0:tabTemplate');
      expect(buildExtendedTabsTemplateInstanceChildId('tabTemplate', 'tabs', 1))
        .assertEqual('tabstabTemplate:1:tabTemplate');
    });

    it('should_report_concrete_values_when_extended_tabs_index_exceeds_item_count', 0, () => {
      let validResult: ExtendedTabsIndexRangeResult = normalizeExtendedTabsIndexForItemCount(2, 3);
      expect(validResult.resolvedIndex).assertEqual(2);
      expect(validResult.shouldReportWarning).assertFalse();
      expect(validResult.warningMessage).assertEqual('');

      let invalidResult: ExtendedTabsIndexRangeResult = normalizeExtendedTabsIndexForItemCount(5, 3);
      expect(invalidResult.resolvedIndex).assertEqual(2);
      expect(invalidResult.shouldReportWarning).assertTrue();
      expect(invalidResult.warningMessage).assertEqual(
        'Property tabIndex value 5 is invalid. Valid values are from 0 to 2. Fallback to 2.'
      );
    });

    it('should_dispatch_extended_tabs_index_range_warning_with_concrete_values', 0, () => {
      let dispatchedWarnings: SchemaErrorInfo[] = [];
      SchemaErrorInfoManager.getInstance().registerWarningEmitter(
        'schemaWarningSurface',
        (warnings: SchemaErrorInfo[]): void => {
          dispatchedWarnings = warnings;
        }
      );
      let attribute: CustomComponentAttribute = createCustomComponentAttribute('Tabs', 'tabsRange');
      dispatchExtendedTabsIndexRangeWarning(
        attribute,
        'Property tabIndex value 5 is invalid. Valid values are from 0 to 2. Fallback to 2.'
      );
      SchemaErrorInfoManager.getInstance().unregisterWarningEmitter('schemaWarningSurface');

      expect(dispatchedWarnings.length).assertEqual(1);
      expect(dispatchedWarnings[0].code).assertEqual(SchemaErrorCode.INVALID_VALUE);
      expect(dispatchedWarnings[0].path).assertEqual('tabsRange.tabIndex');
      expect(dispatchedWarnings[0].message).assertEqual(
        'Property tabIndex value 5 is invalid. Valid values are from 0 to 2. Fallback to 2.'
      );
    });

    it('should_reject_negative_extended_tab_content_metric_results', 0, () => {
      expect(normalizeExtendedTabContentMetricValue(12)).assertEqual(12);
      expect(normalizeExtendedTabContentMetricValue(0)).assertEqual(0);
      expect(normalizeExtendedTabContentMetricValue(-1) === undefined).assertTrue();
      expect(normalizeExtendedTabContentMetricValue(Number.NaN) === undefined).assertTrue();
    });

    it('should_reject_extended_tab_content_font_weight_results_outside_protocol_range', 0, () => {
      expect(normalizeExtendedTabContentFontWeightValue(500) as number).assertEqual(500);
      expect(normalizeExtendedTabContentFontWeightValue('bold') as string).assertEqual('bold');
      expect(normalizeExtendedTabContentFontWeightValue(550) === undefined).assertTrue();
      expect(normalizeExtendedTabContentFontWeightValue(50) === undefined).assertTrue();
    });

    xit('should_expose_expression_and_range_contracts_for_audited_extended_components', 0, () => {
      let catalog: CatalogImpl = CatalogFactoryCore.extended();
      let commonSchema: string = getExtendedCommonSchema();
      let columnSchema: string = getComponentSchema(catalog, 'Column');
      let listSchema: string = getComponentSchema(catalog, 'List');
      let stackSchema: string = getComponentSchema(catalog, 'Stack');
      let gridSchema: string = getComponentSchema(catalog, 'Grid');
      let navSchema: string = getComponentSchema(catalog, 'NavContainer');
      let tabsSchema: string = getComponentSchema(catalog, 'Tabs');
      let tabContentSchema: string = getComponentSchema(catalog, 'TabContent');

      let commonRoot: Record<string, Object> = JSON.parse(commonSchema) as Record<string, Object>;
      let commonDefs: Record<string, Object> = commonRoot['$defs'] as Record<string, Object>;
      let dynamicNumber: string = JSON.stringify(commonDefs['DynamicNumber']);
      let dynamicBoolean: string = JSON.stringify(commonDefs['DynamicBoolean']);
      let extendedDynamicValueRef: string = JSON.stringify(commonDefs['ExtendedDynamicValueRef']);

      expect(commonDefs['Expression'] !== undefined).assertTrue();
      expect(commonDefs['PathBinding'] !== undefined).assertTrue();
      expect(commonDefs['ExtendedDynamicString'] !== undefined).assertTrue();
      expect(commonDefs['ExtendedDynamicNumber'] !== undefined).assertTrue();
      expect(commonDefs['ExtendedDynamicBoolean'] !== undefined).assertTrue();
      expect(dynamicNumber.indexOf('#/$defs/Expression') >= 0).assertTrue();
      expect(dynamicBoolean.indexOf('#/$defs/Expression') >= 0).assertTrue();
      expect(extendedDynamicValueRef.indexOf('#/$defs/Expression') >= 0).assertTrue();
      expect(extendedDynamicValueRef.indexOf('#/$defs/PathBinding') >= 0).assertTrue();
      expect(extendedDynamicValueRef.indexOf('#/$defs/FunctionCall') >= 0).assertTrue();

      expect(columnSchema.indexOf('"NonNegativeNumberValue"') >= 0).assertTrue();
      expect(columnSchema.indexOf('ExtendedDynamicValueRef') >= 0).assertTrue();
      expect(listSchema.indexOf('"NonNegativeNumberValue"') >= 0).assertTrue();
      expect(listSchema.indexOf('"DynamicNestedScrollMode"') >= 0).assertTrue();
      expect(stackSchema.indexOf('ExtendedDynamicValueRef') >= 0).assertTrue();
      expect(gridSchema.indexOf('"NonNegativeNumberValue"') >= 0).assertTrue();
      expect(navSchema.indexOf('"NonNegativeIntegerValue"') >= 0).assertTrue();
      expect(tabsSchema.indexOf('"NonNegativeIntegerValue"') >= 0).assertTrue();
      expect(tabsSchema.indexOf('ExtendedDynamicBoolean') >= 0).assertTrue();
      expect(tabContentSchema.indexOf('"NonNegativeNumberValue"') >= 0).assertTrue();
      expect(columnSchema.indexOf('"ExpressionString"')).assertEqual(-1);
      expect(listSchema.indexOf('"ExpressionString"')).assertEqual(-1);
      expect(stackSchema.indexOf('"ExpressionString"')).assertEqual(-1);
      expect(gridSchema.indexOf('"ExpressionString"')).assertEqual(-1);
      expect(navSchema.indexOf('"ExpressionString"')).assertEqual(-1);
      expect(tabsSchema.indexOf('"ExpressionString"')).assertEqual(-1);
      expect(tabsSchema.indexOf('"DynamicBooleanExpression"')).assertEqual(-1);
      expect(tabContentSchema.indexOf('"ExpressionString"')).assertEqual(-1);
    });

    it('should_keep_disabled_and_report_warning_when_date_time_input_has_no_enabled_dimension', 0, () => {
      let options: DateTimeInputOptions = {
        value: '',
        enableDate: false,
        enableTime: false,
        min: '',
        max: '',
        label: ''
      };

      let normalized: DateTimeInputOptions =
        normalizeDateTimeInputOptionsForSchemaWarning(options, 'dateTimeInputTest');

      expect(normalized.enableDate === true).assertFalse();
      expect(normalized.enableTime === true).assertFalse();
      expect(hasWarning('dateTimeInputTest.enableDate', 'remains disabled')).assertTrue();
    });

    it('should_normalize_parseable_non_iso_date_value_and_report_warning', 0, () => {
      let options: DateTimeInputOptions = {
        value: '2024/01/01',
        enableDate: true,
        enableTime: false,
        min: '',
        max: '',
        label: ''
      };

      let normalized: DateTimeInputOptions =
        normalizeDateTimeInputOptionsForSchemaWarning(options, 'dateTimeInputTest');

      expect(normalized.value).assertEqual('2024-01-01');
      expect(hasWarning('dateTimeInputTest.value', 'value has been normalized')).assertTrue();
    });

    it('should_report_date_time_input_value_warning_once_when_normalized_repeatedly', 0, () => {
      let options: DateTimeInputOptions = {
        value: '2024/01/01',
        enableDate: true,
        enableTime: false,
        min: '',
        max: '',
        label: ''
      };

      normalizeDateTimeInputOptionsForSchemaWarning(options, 'dateTimeInputTest');
      normalizeDateTimeInputOptionsForSchemaWarning(options, 'dateTimeInputTest');

      expect(SchemaErrorInfoManager.getInstance().getWarningCount()).assertEqual(1);
    });

    it('should_drop_invalid_or_inverted_date_time_input_boundaries_and_report_warning', 0, () => {
      let invalidBoundary: DateTimeInputOptions = normalizeDateTimeInputOptionsForSchemaWarning(
        {
          value: '',
          enableDate: true,
          enableTime: false,
          min: 'invalid-date',
          max: '2024-12-31',
          label: ''
        },
        'dateTimeInputTest'
      );
      expect(invalidBoundary.min).assertEqual('');
      expect(invalidBoundary.max).assertEqual('2024-12-31');
      expect(hasWarning('dateTimeInputTest.min', 'field has been ignored')).assertTrue();

      SchemaErrorInfoManager.getInstance().clear();
      let invertedBoundary: DateTimeInputOptions = normalizeDateTimeInputOptionsForSchemaWarning(
        {
          value: '',
          enableDate: true,
          enableTime: false,
          min: '2024-12-31',
          max: '2024-01-01',
          label: ''
        },
        'dateTimeInputTest'
      );
      expect(invertedBoundary.min).assertEqual('');
      expect(invertedBoundary.max).assertEqual('');
      expect(hasWarning('dateTimeInputTest.min', 'must not be later than max')).assertTrue();
    });

    it('should_validate_audio_and_video_url_resource_formats', 0, () => {
      expect(isSupportedAudioUrl('https://example.com/voice.mp3?token=1')).assertTrue();
      expect(isSupportedAudioUrl('https://example.com/not-audio.txt')).assertFalse();
      expect(isSupportedVideoUrl('https://example.com/movie.mp4#preview')).assertTrue();
      expect(isSupportedVideoUrl('https://example.com/not-video.png')).assertFalse();
    });

    it('should_drop_audio_player_and_report_warning_when_url_is_not_audio_resource', 0, () => {
      let options: AudioPlayerOptions = {
        url: 'https://example.com/not-audio.txt',
        description: '',
        renderContent: true
      };

      let normalized: AudioPlayerOptions = normalizeAudioPlayerOptionsForSchemaWarning(options, 'audioInvalid.url');

      expect(normalized.renderContent).assertFalse();
      expect(normalized.url).assertEqual('');
      expect(hasWarning('audioInvalid.url', 'drop current audioPlayer component')).assertTrue();
    });

    it('should_report_required_warning_when_video_custom_props_are_absent', 0, () => {
      let options = resolveVideoOptionsForSchemaWarning(createCustomComponentAttribute('Video', 'videoMissing'));

      expect(options.url).assertEqual('');
      expect(options.renderContent).assertTrue();
      expect(hasWarning('videoMissing.url', 'Property url is required')).assertTrue();
    });

    it('should_report_video_type_mismatch_warning_with_component_id_path', 0, () => {
      let videoCustomProps: Record<string, Object> = new Object() as Record<string, Object>;
      videoCustomProps['url'] = 12345 as Object;
      let options = resolveVideoOptionsForSchemaWarning(
        createCustomComponentAttribute('Video', 'target', videoCustomProps)
      );

      expect(options.url).assertEqual('');
      expect(hasWarning('target.url', 'fallback to default value')).assertTrue();
    });

    it('should_report_required_warning_when_audio_player_custom_props_are_absent', 0, () => {
      let options: AudioPlayerOptions = resolveAudioPlayerOptionsForSchemaWarning(
        createCustomComponentAttribute('AudioPlayer', 'audioMissing')
      );

      expect(options.url).assertEqual('');
      expect(options.renderContent).assertTrue();
      expect(hasWarning('audioMissing.url', 'Property url is required')).assertTrue();
    });

    it('should_report_icon_warning_with_component_id_path', 0, () => {
      let iconCustomProps: Record<string, Object> = new Object() as Record<string, Object>;
      iconCustomProps['name'] = 1024 as Object;
      resolveIconRenderState(createCustomComponentAttribute('Icon', 'iconTarget', iconCustomProps));

      expect(hasWarning('iconTarget.name', 'fallback')).assertTrue();
    });

    it('should_report_extended_select_options_symbol_icon_schema_warnings', 0, () => {
      let symbolIcon: Record<string, Object> = {};
      symbolIcon['fontSize'] = -1 as Object;
      symbolIcon['fontColor'] = 'bad-color' as Object;
      symbolIcon['renderingStrategy'] = 'invalid' as Object;

      let option: Record<string, Object> = {};
      option['symbolIcon'] = symbolIcon as Object;

      let invalidSrcSymbolIcon: Record<string, Object> = {};
      invalidSrcSymbolIcon['src'] = 'sys.symbol.ohos_star' as Object;
      let invalidSrcOption: Record<string, Object> = {};
      invalidSrcOption['value'] = 'B' as Object;
      invalidSrcOption['symbolIcon'] = invalidSrcSymbolIcon as Object;

      let options: Object[] = [option as Object, invalidSrcOption as Object];
      let customProps: Record<string, Object> = {};
      customProps['options'] = options as Object;
      customProps['selected'] = 'bad' as Object;

      validateExtendedSelectSchemaForWarning({
        type: 'Select',
        id: 'selectTarget',
        customProps: customProps
      } as CustomComponentAttribute);

      expect(hasWarning('selectTarget.options[0].value', 'required')).assertTrue();
      expect(hasWarning('selectTarget.options[0].symbolIcon.src', 'required')).assertFalse();
      expect(hasWarning('selectTarget.options[0].symbolIcon.fontSize', 'out of range')).assertTrue();
      expect(hasWarningCode('selectTarget.options[0].symbolIcon.fontColor', SchemaErrorCode.INVALID_VALUE))
        .assertTrue();
      expect(hasWarning('selectTarget.options[0].symbolIcon.renderingStrategy', 'enum range')).assertTrue();
      expect(hasWarning('selectTarget.options[1].symbolIcon.src', 'enum range')).assertTrue();
      expect(hasWarning('selectTarget.selected', 'expects number')).assertTrue();
    });

    it('should_report_extended_select_invalid_color_and_font_enum_schema_warnings', 0, () => {
      let symbolIcon: Record<string, Object> = {};
      symbolIcon['src'] = 'star' as Object;
      symbolIcon['fontColor'] = 'bad-color' as Object;

      let option: Record<string, Object> = {};
      option['value'] = 'A' as Object;
      option['symbolIcon'] = symbolIcon as Object;
      let options: Object[] = [option as Object];

      let font: Record<string, Object> = {};
      font['weight'] = 'ultraHeavy' as Object;
      font['style'] = 'oblique' as Object;

      let styles: Record<string, Object> = {};
      styles['font'] = font as Object;
      styles['selectedOptionBgColor'] = 'bad-color' as Object;
      styles['selectedOptionFontColor'] = 'bad-color' as Object;
      styles['menuBackgroundColor'] = 'bad-color' as Object;
      let divider: Record<string, Object> = {};
      divider['color'] = 'bad-color' as Object;
      styles['divider'] = divider as Object;

      let customProps: Record<string, Object> = {};
      customProps['options'] = options as Object;
      customProps['value'] = 'A' as Object;
      customProps['styles'] = styles as Object;

      validateExtendedSelectSchemaForWarning({
        type: 'Select',
        id: 'selectInvalidColorTarget',
        customProps: customProps
      } as CustomComponentAttribute);

      expect(hasWarningCode(
        'selectInvalidColorTarget.options[0].symbolIcon.fontColor',
        SchemaErrorCode.INVALID_VALUE
      )).assertTrue();
      expect(hasWarningCode('selectInvalidColorTarget.styles.font.weight', SchemaErrorCode.INVALID_VALUE))
        .assertTrue();
      expect(hasWarningCode('selectInvalidColorTarget.styles.font.style', SchemaErrorCode.INVALID_VALUE))
        .assertTrue();
      expect(hasWarningCode('selectInvalidColorTarget.styles.selectedOptionBgColor', SchemaErrorCode.INVALID_VALUE))
        .assertTrue();
      expect(hasWarningCode('selectInvalidColorTarget.styles.selectedOptionFontColor', SchemaErrorCode.INVALID_VALUE))
        .assertTrue();
      expect(hasWarningCode('selectInvalidColorTarget.styles.menuBackgroundColor', SchemaErrorCode.INVALID_VALUE))
        .assertTrue();
      expect(hasWarningCode('selectInvalidColorTarget.styles.divider.color', SchemaErrorCode.INVALID_VALUE))
        .assertTrue();
    });

    it('should_allow_extended_select_selected_minus_one_without_warning', 0, () => {
      let option: Record<string, Object> = {};
      option['value'] = 'A' as Object;
      let options: Object[] = [option as Object];

      let customProps: Record<string, Object> = {};
      customProps['options'] = options as Object;
      customProps['selected'] = -1 as Object;
      customProps['value'] = 'A' as Object;

      validateExtendedSelectSchemaForWarning({
        type: 'Select',
        id: 'selectNoSelectionTarget',
        customProps: customProps
      } as CustomComponentAttribute);

      expect(SchemaErrorInfoManager.getInstance().getWarningCount()).assertEqual(0);
    });

    it('should_allow_extended_select_lowercase_enum_and_font_family_style_without_warning', 0, () => {
      let symbolIcon: Record<string, Object> = {};
      symbolIcon['src'] = 'star' as Object;

      let option: Record<string, Object> = {};
      option['value'] = 'A' as Object;
      option['symbolIcon'] = symbolIcon as Object;
      let options: Object[] = [option as Object];

      let selectedOptionFont: Record<string, Object> = {};
      selectedOptionFont['size'] = 16 as Object;
      selectedOptionFont['weight'] = 'Bold' as Object;
      selectedOptionFont['family'] = 'HarmonyOS Sans' as Object;
      selectedOptionFont['style'] = 'italic' as Object;

      let optionFont: Record<string, Object> = {};
      optionFont['size'] = 14 as Object;
      optionFont['weight'] = 'Regular' as Object;
      optionFont['family'] = 'sans-serif' as Object;
      optionFont['style'] = 'normal' as Object;

      let menuAlign: Record<string, Object> = {};
      menuAlign['alignType'] = 'center' as Object;

      let styles: Record<string, Object> = {};
      styles['selectedOptionFont'] = selectedOptionFont as Object;
      styles['optionFont'] = optionFont as Object;
      styles['arrowPosition'] = 'start' as Object;
      styles['menuAlign'] = menuAlign as Object;

      let customProps: Record<string, Object> = {};
      customProps['options'] = options as Object;
      customProps['value'] = 'A' as Object;
      customProps['styles'] = styles as Object;

      validateExtendedSelectSchemaForWarning({
        type: 'Select',
        id: 'selectLowercaseTarget',
        customProps: customProps
      } as CustomComponentAttribute);

      expect(SchemaErrorInfoManager.getInstance().getWarningCount()).assertEqual(0);
    });

    it('should_allow_extended_select_dynamic_private_attributes_and_styles_without_warning', 0, () => {
      let font: Record<string, Object> = {};
      font['size'] = createPathBinding('/fontSize') as Object;
      font['weight'] = createPathBinding('/fontWeight') as Object;
      font['family'] = createPathBinding('/fontFamily') as Object;
      font['style'] = createPathBinding('/fontStyle') as Object;

      let menuAlign: Record<string, Object> = {};
      menuAlign['alignType'] = createPathBinding('/alignType') as Object;
      menuAlign['offset'] = createPathBinding('/offset') as Object;

      let divider: Record<string, Object> = {};
      divider['strokeWidth'] = createPathBinding('/dividerStrokeWidth') as Object;
      divider['color'] = createPathBinding('/dividerColor') as Object;

      let styles: Record<string, Object> = {};
      styles['font'] = font as Object;
      styles['fontColor'] = createPathBinding('/fontColor') as Object;
      styles['space'] = createPathBinding('/space') as Object;
      styles['arrowPosition'] = createPathBinding('/arrowPosition') as Object;
      styles['menuAlign'] = menuAlign as Object;
      styles['optionWidth'] = createPathBinding('/optionWidth') as Object;
      styles['optionHeight'] = createPathBinding('/optionHeight') as Object;
      styles['divider'] = divider as Object;

      let customProps: Record<string, Object> = {};
      customProps['options'] = createPathBinding('/selectOptions') as Object;
      customProps['selected'] = createPathBinding('/selectedIndex') as Object;
      customProps['value'] = createPathBinding('/selectValue') as Object;
      customProps['styles'] = styles as Object;

      validateExtendedSelectSchemaForWarning({
        type: 'Select',
        id: 'selectDynamicTarget',
        customProps: customProps
      } as CustomComponentAttribute);

      expect(SchemaErrorInfoManager.getInstance().getWarningCount()).assertEqual(0);
    });

    it('should_allow_expression_result_object_for_extended_select_menu_align_offset', 0, () => {
      let menuAlign: Record<string, Object> = {};
      menuAlign['alignType'] = 'start' as Object;
      menuAlign['offset'] = '{{ $__dataModel.menuOffset }}' as Object;

      let styles: Record<string, Object> = {};
      styles['menuAlign'] = menuAlign as Object;

      let customProps: Record<string, Object> = {};
      let option: Record<string, Object> = {};
      option['value'] = 'A' as Object;
      customProps['options'] = [option as Object] as Object;
      customProps['value'] = 'A' as Object;
      customProps['styles'] = styles as Object;

      let dataModel: ESObject = {} as ESObject;
      let menuOffset: Record<string, Object> = {};
      menuOffset['dx'] = 12 as Object;
      menuOffset['dy'] = 8 as Object;
      dataModel['menuOffset'] = menuOffset as Object;

      validateExtendedSelectSchemaForWarning({
        type: 'Select',
        id: 'selectExpressionOffsetTarget',
        customProps: customProps,
        dataModel: dataModel
      } as CustomComponentAttribute);

      expect(SchemaErrorInfoManager.getInstance().getWarningCount()).assertEqual(0);
    });

    it('should_report_extended_select_dynamic_result_dfx_warnings_in_schema_format', 0, () => {
      let styles: Record<string, Object> = {};
      let spaceBinding: Record<string, Object> = {};
      spaceBinding['path'] = '/space';
      let arrowBinding: Record<string, Object> = {};
      arrowBinding['path'] = '/arrowPosition';
      styles['space'] = spaceBinding as Object;
      styles['arrowPosition'] = arrowBinding as Object;
      styles['font'] = createPathBinding('/font') as Object;
      styles['selectedOptionBgColor'] = createPathBinding('/selectedOptionBgColor') as Object;
      styles['selectedOptionFontColor'] = createPathBinding('/selectedOptionFontColor') as Object;
      styles['menuBackgroundColor'] = createPathBinding('/menuBackgroundColor') as Object;
      styles['menuAlign'] = createPathBinding('/menuAlign') as Object;
      styles['divider'] = createPathBinding('/divider') as Object;

      let customProps: Record<string, Object> = {};
      customProps['options'] = createPathBinding('/selectOptions') as Object;
      customProps['selected'] = createPathBinding('/selected') as Object;
      customProps['value'] = 'A' as Object;
      customProps['styles'] = styles as Object;
      let symbolIcon: Record<string, Object> = {};
      symbolIcon['src'] = 'sys.symbol.ohos_star' as Object;
      symbolIcon['fontColor'] = 'bad-color' as Object;
      let option: Record<string, Object> = {};
      option['value'] = 'A' as Object;
      option['symbolIcon'] = symbolIcon as Object;
      let selectOptions: Object[] = [option as Object];
      let font: Record<string, Object> = {};
      font['weight'] = 'ultraHeavy' as Object;
      font['style'] = 'oblique' as Object;
      let offset: Record<string, Object> = {};
      offset['invalid'] = 'bad-offset' as Object;
      let menuAlign: Record<string, Object> = {};
      menuAlign['offset'] = offset as Object;
      let divider: Record<string, Object> = {};
      divider['color'] = 'bad-color' as Object;
      let dataModel: ESObject = {} as ESObject;
      dataModel['selectOptions'] = selectOptions as Object;
      dataModel['selected'] = '2' as Object;
      dataModel['space'] = -1 as Object;
      dataModel['arrowPosition'] = 'middle' as Object;
      dataModel['font'] = font as Object;
      dataModel['selectedOptionBgColor'] = 'bad-color' as Object;
      dataModel['selectedOptionFontColor'] = 'bad-color' as Object;
      dataModel['menuBackgroundColor'] = 'bad-color' as Object;
      dataModel['menuAlign'] = menuAlign as Object;
      dataModel['divider'] = divider as Object;

      validateExtendedSelectSchemaForWarning({
        type: 'Select',
        id: 'selectDynamicDfxTarget',
        customProps: customProps,
        dataModel: dataModel
      } as CustomComponentAttribute);

      expect(hasWarningCode('selectDynamicDfxTarget.selected', SchemaErrorCode.TYPE_MISMATCH)).assertTrue();
      expect(hasWarningCode('selectDynamicDfxTarget.options[0].symbolIcon.src', SchemaErrorCode.INVALID_VALUE))
        .assertTrue();
      expect(hasWarningCode(
        'selectDynamicDfxTarget.options[0].symbolIcon.fontColor',
        SchemaErrorCode.INVALID_VALUE
      )).assertTrue();
      expect(hasWarningCode('selectDynamicDfxTarget.styles.space', SchemaErrorCode.INVALID_VALUE)).assertTrue();
      expect(hasWarningCode('selectDynamicDfxTarget.styles.arrowPosition', SchemaErrorCode.INVALID_VALUE))
        .assertTrue();
      expect(hasWarningCode('selectDynamicDfxTarget.styles.font.weight', SchemaErrorCode.INVALID_VALUE))
        .assertTrue();
      expect(hasWarningCode('selectDynamicDfxTarget.styles.font.style', SchemaErrorCode.INVALID_VALUE))
        .assertTrue();
      expect(hasWarningCode('selectDynamicDfxTarget.styles.selectedOptionBgColor', SchemaErrorCode.INVALID_VALUE))
        .assertTrue();
      expect(hasWarningCode('selectDynamicDfxTarget.styles.selectedOptionFontColor', SchemaErrorCode.INVALID_VALUE))
        .assertTrue();
      expect(hasWarningCode('selectDynamicDfxTarget.styles.menuBackgroundColor', SchemaErrorCode.INVALID_VALUE))
        .assertTrue();
      expect(hasWarningCode('selectDynamicDfxTarget.styles.menuAlign.offset.invalid',
        SchemaErrorCode.UNDEFINED_FIELD)).assertTrue();
      expect(hasWarningCode('selectDynamicDfxTarget.styles.divider.color', SchemaErrorCode.INVALID_VALUE))
        .assertTrue();
    });

    it('should_report_extended_select_private_style_schema_warnings', 0, () => {
      let font: Record<string, Object> = {};
      font['size'] = 0 as Object;
      font['family'] = 42 as Object;
      font['style'] = false as Object;

      let menuAlign: Record<string, Object> = {};
      menuAlign['alignType'] = 'bad' as Object;

      let styles: Record<string, Object> = {};
      styles['font'] = font as Object;
      styles['arrowPosition'] = 'BAD' as Object;
      styles['menuAlign'] = menuAlign as Object;
      styles['divider'] = false as Object;

      let option: Record<string, Object> = {};
      option['value'] = 'A' as Object;
      let options: Object[] = [option as Object];

      let customProps: Record<string, Object> = {};
      customProps['options'] = options as Object;
      customProps['value'] = 'A' as Object;
      customProps['styles'] = styles as Object;

      validateExtendedSelectSchemaForWarning({
        type: 'Select',
        id: 'selectStyleTarget',
        customProps: customProps
      } as CustomComponentAttribute);

      expect(hasWarning('selectStyleTarget.styles.font.size', 'out of range')).assertTrue();
      expect(hasWarning('selectStyleTarget.styles.font.family', 'expects string')).assertTrue();
      expect(hasWarning('selectStyleTarget.styles.font.style', 'expects string')).assertTrue();
      expect(hasWarning('selectStyleTarget.styles.arrowPosition', 'enum range')).assertTrue();
      expect(hasWarning('selectStyleTarget.styles.menuAlign.alignType', 'enum range')).assertTrue();
      expect(hasWarning('selectStyleTarget.styles.divider', 'expects object or null')).assertTrue();
    });

    xit('should_allow_extended_select_empty_optional_object_schema_values', 0, () => {
      let styles: Record<string, Object> = {};
      styles['font'] = new Object() as Object;
      styles['selectedOptionFont'] = new Object() as Object;
      styles['optionFont'] = new Object() as Object;
      styles['menuAlign'] = new Object() as Object;
      styles['divider'] = new Object() as Object;

      let emptyOption: Record<string, Object> = new Object() as Record<string, Object>;
      let options: Object[] = [emptyOption as Object];

      let customProps: Record<string, Object> = {};
      customProps['options'] = options as Object;
      customProps['value'] = 'A' as Object;
      customProps['styles'] = styles as Object;

      validateExtendedSelectSchemaForWarning({
        type: 'Select',
        id: 'selectEmptyObjectTarget',
        customProps: customProps
      } as CustomComponentAttribute);

      expect(hasWarningCode('selectEmptyObjectTarget.options[0].value', SchemaErrorCode.REQUIRED_MISS)).assertFalse();
      expect(hasWarning('selectEmptyObjectTarget.styles.font', 'at least one supported field')).assertFalse();
      expect(hasWarning('selectEmptyObjectTarget.styles.selectedOptionFont', 'at least one supported field'))
        .assertFalse();
      expect(hasWarning('selectEmptyObjectTarget.styles.optionFont', 'at least one supported field')).assertFalse();
      expect(hasWarning('selectEmptyObjectTarget.styles.menuAlign', 'at least one supported field')).assertFalse();
      expect(hasWarning('selectEmptyObjectTarget.styles.divider', 'at least one supported field')).assertFalse();
    });

    it('should_report_extended_select_unknown_optional_object_members', 0, () => {
      let font: Record<string, Object> = {};
      font['invalid'] = 'bad-font' as Object;
      let menuAlign: Record<string, Object> = {};
      menuAlign['invalid'] = 'bad-menu-align' as Object;
      let menuAlignOffset: Record<string, Object> = {};
      menuAlignOffset['invalid'] = 'bad-offset' as Object;
      menuAlign['offset'] = menuAlignOffset as Object;
      let divider: Record<string, Object> = {};
      divider['invalid'] = 'bad-divider' as Object;
      let option: Record<string, Object> = {};
      option['invalid'] = 'bad-option' as Object;
      let options: Object[] = [option as Object];

      let styles: Record<string, Object> = {};
      styles['font'] = font as Object;
      styles['selectedOptionFont'] = font as Object;
      styles['optionFont'] = font as Object;
      styles['menuAlign'] = menuAlign as Object;
      styles['divider'] = divider as Object;

      let customProps: Record<string, Object> = {};
      customProps['options'] = options as Object;
      customProps['value'] = 'A' as Object;
      customProps['styles'] = styles as Object;

      validateExtendedSelectSchemaForWarning({
        type: 'Select',
        id: 'selectUnknownObjectTarget',
        customProps: customProps
      } as CustomComponentAttribute);

      expect(hasWarningCode('selectUnknownObjectTarget.options[0].invalid', SchemaErrorCode.UNDEFINED_FIELD))
        .assertTrue();
      expect(hasWarningCode('selectUnknownObjectTarget.styles.font.invalid', SchemaErrorCode.UNDEFINED_FIELD))
        .assertTrue();
      expect(hasWarningCode(
        'selectUnknownObjectTarget.styles.selectedOptionFont.invalid',
        SchemaErrorCode.UNDEFINED_FIELD
      )).assertTrue();
      expect(hasWarningCode('selectUnknownObjectTarget.styles.optionFont.invalid', SchemaErrorCode.UNDEFINED_FIELD))
        .assertTrue();
      expect(hasWarningCode('selectUnknownObjectTarget.styles.menuAlign.invalid', SchemaErrorCode.UNDEFINED_FIELD))
        .assertTrue();
      expect(hasWarningCode('selectUnknownObjectTarget.styles.menuAlign.offset.invalid',
        SchemaErrorCode.UNDEFINED_FIELD)).assertTrue();
      expect(hasWarningCode('selectUnknownObjectTarget.styles.divider.invalid', SchemaErrorCode.UNDEFINED_FIELD))
        .assertTrue();
    });

    it('should_keep_extended_select_options_array_type_validation', 0, () => {
      let customProps: Record<string, Object> = {};
      customProps['options'] = new Object() as Object;
      customProps['value'] = 'A' as Object;

      validateExtendedSelectSchemaForWarning({
        type: 'Select',
        id: 'selectOptionsTypeTarget',
        customProps: customProps
      } as CustomComponentAttribute);

      expect(hasWarning('selectOptionsTypeTarget.options', 'expects array')).assertTrue();
    });

    it('should_reject_object_value_for_non_empty_string_property_without_coercion', 0, () => {
      let childValue: Record<string, Object> = new Object() as Record<string, Object>;
      childValue['value'] = 'content4' as Object;
      let source: Record<string, Object> = new Object() as Record<string, Object>;
      source['child'] = childValue as Object;

      let normalized: string | undefined =
        SchemaLocalPropertyHelper.normalizeNonEmptyStringProperty(source, 'child', 'Tabs', 'tabsTest', true);

      expect(normalized === undefined).assertTrue();
      expect(hasWarning('tabsTest.child', 'expects non-empty string')).assertTrue();
      expect(hasWarning('tabsTest.child', 'coerced')).assertFalse();
    });

    it('should_report_warning_when_object_array_is_smaller_than_min_items', 0, () => {
      let source: Record<string, Object> = new Object() as Record<string, Object>;
      source['tabs'] = [] as Object;

      let normalized: Record<string, Object>[] =
        SchemaLocalPropertyHelper.normalizeObjectArrayProperty(source, 'tabs', 'Tabs', 'tabsTest', true, 1);

      expect(normalized.length).assertEqual(0);
      expect(hasWarning('tabsTest.tabs', 'schema minimum')).assertTrue();
    });

    it('should_expose_extended_text_schema_with_content_and_font_scale_fields', 0, () => {
      let catalog: CatalogImpl = CatalogFactoryCore.extended();
      let schema: string = getComponentSchema(catalog, 'Text');

      expect(schema.length > 0).assertTrue();
      expect(SchemaValidator.validateComponentSchema('Text', schema).length).assertEqual(0);
      expect(schema.indexOf('"content"') >= 0).assertTrue();
      expect(schema.indexOf('"styles"') >= 0).assertTrue();
      expect(schema.indexOf('"fontScaleMode"') >= 0).assertTrue();
      expect(schema.indexOf('"minFontScale"') >= 0).assertTrue();
      expect(schema.indexOf('"maxFontScale"') >= 0).assertTrue();
      expect(schema.indexOf('"decoration"') >= 0).assertTrue();
    });

    xit('should_expose_extended_text_decoration_member_expression_branches', 0, () => {
      let catalog: CatalogImpl = CatalogFactoryCore.extended();
      let schema: string = getComponentSchema(catalog, 'Text');
      let root: Record<string, Object> = JSON.parse(schema) as Record<string, Object>;
      let defs: Record<string, Object> = root['$defs'] as Record<string, Object>;
      let decorationLiteral: Record<string, Object> = defs['DecorationLiteral'] as Record<string, Object>;
      let decorationProperties: Record<string, Object> =
        decorationLiteral['properties'] as Record<string, Object>;
      let dynamicType: string = JSON.stringify(defs['DynamicDecorationType']);
      let dynamicStyle: string = JSON.stringify(defs['DynamicDecorationStyle']);
      let dynamicStringOrNumber: string = JSON.stringify(defs['DynamicDecorationStringOrNumber']);

      expect(defs['Expression'] !== undefined).assertTrue();
      expect(JSON.stringify(decorationProperties['type']).indexOf('#/$defs/DynamicDecorationType') >= 0).assertTrue();
      expect(JSON.stringify(decorationProperties['color']).indexOf('#/$defs/DynamicDecorationStringOrNumber') >= 0)
        .assertTrue();
      expect(JSON.stringify(decorationProperties['style']).indexOf('#/$defs/DynamicDecorationStyle') >= 0)
        .assertTrue();
      expect(JSON.stringify(decorationProperties['thicknessScale'])
        .indexOf('#/$defs/DynamicDecorationStringOrNumber') >= 0).assertTrue();
      expect(dynamicType.indexOf('#/$defs/Expression') >= 0).assertTrue();
      expect(dynamicType.indexOf('#/$defs/DataBinding') >= 0).assertTrue();
      expect(dynamicStyle.indexOf('#/$defs/Expression') >= 0).assertTrue();
      expect(dynamicStyle.indexOf('#/$defs/DataBinding') >= 0).assertTrue();
      expect(dynamicStringOrNumber.indexOf('#/$defs/Expression') >= 0).assertTrue();
      expect(dynamicStringOrNumber.indexOf('#/$defs/DataBinding') >= 0).assertTrue();
    });

    xit('should_expose_extended_common_style_defs_for_shared_modifier_fields_when_requested', 0, () => {
      let schema: string = getExtendedCommonSchema();

      expect(schema.length > 0).assertTrue();
      expect(schema.indexOf('"ExtendedCommonStyles"') >= 0).assertTrue();
      expect(schema.indexOf('"backgroundImageSizeWithStyle"') >= 0).assertTrue();
      expect(schema.indexOf('"backgroundimageSize"') >= 0).assertTrue();
      expect(schema.indexOf('"constraintSize"') >= 0).assertTrue();
      expect(schema.indexOf('"layoutWeight"') >= 0).assertTrue();
      expect(schema.indexOf('"flexShrink"') >= 0).assertTrue();
      expect(schema.indexOf('"shadow"') >= 0).assertTrue();
      expect(schema.indexOf('"linearGradient"') >= 0).assertTrue();
    });

    xit('should_expose_extended_tabs_schema_with_literal_tab_index_and_on_change_wrapper_when_requested', 0, () => {
      let catalog: CatalogImpl = CatalogFactoryCore.extended();
      let schema: string = getComponentSchema(catalog, 'Tabs');
      let commonSchema: string = getExtendedCommonSchema();

      expect(schema.length > 0).assertTrue();
      expect(SchemaValidator.validateComponentSchema('Tabs', schema).length).assertEqual(0);
      expect(schema.indexOf('"tabIndex"') >= 0).assertTrue();
      expect(schema.indexOf('"functionCall"') >= 0).assertTrue();
      expect(schema.indexOf('ExtendedDynamicValueRef') >= 0).assertTrue();
      expect(schema.indexOf('ExtendedDynamicBoolean') >= 0).assertTrue();
      expect(schema.indexOf('"ExpressionString"')).assertEqual(-1);
      expect(commonSchema.indexOf('"DynamicBooleanLike"') >= 0).assertTrue();
      expect(commonSchema.indexOf('"DynamicNumericLike"') >= 0).assertTrue();
      expect(schema.indexOf('"DynamicBarPosition"') >= 0).assertTrue();
      expect(schema.indexOf('ExtendedCommonStyles') >= 0).assertTrue();
    });

    xit('should_expose_extended_tab_content_schema_with_protocol_styles_and_bindings_when_requested', 0, () => {
      let catalog: CatalogImpl = CatalogFactoryCore.extended();
      let schema: string = getComponentSchema(catalog, 'TabContent');

      expect(schema.length > 0).assertTrue();
      expect(SchemaValidator.validateComponentSchema('TabContent', schema).length).assertEqual(0);
      expect(schema.indexOf('"__a2uiBindings"') >= 0).assertTrue();
      expect(schema.indexOf('"selectedColor"') >= 0).assertTrue();
      expect(schema.indexOf('"unSelectedColor"') >= 0).assertTrue();
      expect(schema.indexOf('"defaultBackgroundColor"') >= 0).assertTrue();
      expect(schema.indexOf('"selectedBackgroundColor"') >= 0).assertTrue();
      expect(schema.indexOf('"defaultBorderColor"') >= 0).assertTrue();
      expect(schema.indexOf('"selectedBorderColor"') >= 0).assertTrue();
      expect(schema.indexOf('"selectBackgroundColor"') >= 0).assertFalse();
      expect(schema.indexOf('"selectBorderColor"') >= 0).assertFalse();
      expect(schema.indexOf('"fontWeight"') >= 0).assertTrue();
      expect(schema.indexOf('ExtendedDynamicValueRef') >= 0).assertTrue();
      expect(schema.indexOf('"ExpressionString"')).assertEqual(-1);
      expect(schema.indexOf('"DynamicTabType"') >= 0).assertTrue();
      expect(schema.indexOf('ExtendedCommonStyles') >= 0).assertTrue();
    });

    xit('should_expose_extended_web_schema_with_url_and_size_styles_when_requested', 0, () => {
      let catalog: CatalogImpl = CatalogFactoryCore.extended();
      let schema: string = getComponentSchema(catalog, 'Web');

      expect(schema.length > 0).assertTrue();
      expect(SchemaValidator.validateComponentSchema('Web', schema).length).assertEqual(0);
      expect(schema.indexOf('"url"') >= 0).assertTrue();
      expect(schema.indexOf('ExtendedCommonStyles') >= 0).assertTrue();
    });

    it('should_resolve_zero_layout_weight_and_zero_constraint_size_for_extended_common_styles', 0, () => {
      let options: ExtendedCommonStyleOptions = resolveExtendedCommonStyleOptions(
        createCustomComponentAttribute('Tabs', 'tabsZeroStyles', createZeroLayoutWeightAndConstraintSizeProps())
      );

      expect(options.layoutWeight).assertEqual(0);
      expect(options.constraintSize !== undefined).assertTrue();
      if (options.constraintSize === undefined) {
        return;
      }
      let constraintSize = options.constraintSize as Record<string, Object>;
      expect(constraintSize['minWidth']).assertEqual(0);
      expect(constraintSize['maxWidth']).assertEqual(0);
      expect(constraintSize['minHeight']).assertEqual(0);
      expect(constraintSize['maxHeight']).assertEqual(0);
      expect(options.width).assertEqual(0);
      expect(options.height).assertEqual(0);
      expect(options.padding).assertEqual(0);
      expect(options.borderWidth).assertEqual(0);
    });

    it('should_use_component_resolver_for_extended_common_style_expressions', 0, () => {
      let styles: Record<string, Object> = {
        'width': '{{ widthExpression }}',
        'flexShrink': '{{ positiveFlexShrinkExpression }}'
      };
      let customProps: Record<string, Object> = {
        'styles': styles
      };
      let options: ExtendedCommonStyleOptions = resolveExtendedCommonStyleOptions(
        createCustomComponentAttribute(
          'Tabs',
          'tabsExpressionStyles',
          customProps,
          new CommonStyleExpressionResolverStub()
        )
      );

      expect(options.width).assertEqual(240);
      expect(options.flexShrink).assertEqual(20);
    });

    it('should_restore_invalid_extended_background_image_size_expressions_to_auto', 0, () => {
      let failedStyles: Record<string, Object> = {
        'backgroundImageSizeWithStyle': '{{ invalidBackgroundSizeExpression }}'
      };
      let wrongTypeStyles: Record<string, Object> = {
        'backgroundImageSizeWithStyle': '{{ wrongBackgroundSizeExpression }}'
      };
      let failedOptions: ExtendedCommonStyleOptions = resolveExtendedCommonStyleOptions(
        createCustomComponentAttribute(
          'Tabs',
          'tabsFailedBackgroundSize',
          { 'styles': failedStyles },
          new CommonStyleExpressionResolverStub()
        )
      );
      let wrongTypeOptions: ExtendedCommonStyleOptions = resolveExtendedCommonStyleOptions(
        createCustomComponentAttribute(
          'Tabs',
          'tabsWrongBackgroundSize',
          { 'styles': wrongTypeStyles },
          new CommonStyleExpressionResolverStub()
        )
      );

      expect(failedOptions.backgroundImageSize).assertEqual(ImageSize.Auto);
      expect(wrongTypeOptions.backgroundImageSize).assertEqual(ImageSize.Auto);
    });

    it('should_keep_background_image_size_ratio_when_one_dimension_is_empty_string', 0, () => {
      let widthEmptyOptions: ExtendedCommonStyleOptions = resolveExtendedCommonStyleOptions(
        createCustomComponentAttribute('Tabs', 'tabsEmptyWidthSize',
          createBackgroundImageSizeWithStyleProps('' as Object, '60vp' as Object))
      );
      let heightEmptyOptions: ExtendedCommonStyleOptions = resolveExtendedCommonStyleOptions(
        createCustomComponentAttribute('Tabs', 'tabsEmptyHeightSize',
          createBackgroundImageSizeWithStyleProps('60vp' as Object, '' as Object))
      );

      expect(typeof widthEmptyOptions.backgroundImageSize === 'object').assertTrue();
      let widthEmptySize: Record<string, Object> = widthEmptyOptions.backgroundImageSize as Record<string, Object>;
      expect(widthEmptySize['width']).assertEqual(0);
      expect(widthEmptySize['height']).assertEqual('60vp');

      expect(typeof heightEmptyOptions.backgroundImageSize === 'object').assertTrue();
      let heightEmptySize: Record<string, Object> = heightEmptyOptions.backgroundImageSize as Record<string, Object>;
      expect(heightEmptySize['width']).assertEqual('60vp');
      expect(heightEmptySize['height']).assertEqual(0);
    });

    it('should_match_explicit_zero_dimension_when_other_dimension_is_empty_string', 0, () => {
      let widthEmptyOptions: ExtendedCommonStyleOptions = resolveExtendedCommonStyleOptions(
        createCustomComponentAttribute('Tabs', 'tabsEmptyWidthMatch',
          createBackgroundImageSizeWithStyleProps('' as Object, '60vp' as Object))
      );
      let widthZeroOptions: ExtendedCommonStyleOptions = resolveExtendedCommonStyleOptions(
        createCustomComponentAttribute('Tabs', 'tabsZeroWidthMatch',
          createBackgroundImageSizeWithStyleProps(0 as Object, '60vp' as Object))
      );

      let widthEmptySize: Record<string, Object> = widthEmptyOptions.backgroundImageSize as Record<string, Object>;
      let widthZeroSize: Record<string, Object> = widthZeroOptions.backgroundImageSize as Record<string, Object>;
      expect(widthEmptySize['width']).assertEqual(widthZeroSize['width']);
      expect(widthEmptySize['height']).assertEqual(widthZeroSize['height']);
    });

    it('should_accept_only_string_colors_for_tabs_common_background_color', 0, () => {
      let stringColorStyles: Record<string, Object> = { 'backgroundColor': '#FF102030' };
      let numberColorStyles: Record<string, Object> = { 'backgroundColor': 0xFF102030 as Object };
      let dynamicNumberColorStyles: Record<string, Object> = {
        'backgroundColor': '{{ numericColorExpression }}'
      };
      let stringColorProps: Record<string, Object> = { 'styles': stringColorStyles as Object };
      let numberColorProps: Record<string, Object> = { 'styles': numberColorStyles as Object };
      let dynamicNumberColorProps: Record<string, Object> = { 'styles': dynamicNumberColorStyles as Object };
      let validOptions: ExtendedCommonStyleOptions = resolveExtendedCommonStyleOptions(
        createCustomComponentAttribute('Tabs', 'tabsStringColor', stringColorProps)
      );
      let literalNumberOptions: ExtendedCommonStyleOptions = resolveExtendedCommonStyleOptions(
        createCustomComponentAttribute('Tabs', 'tabsNumberColor', numberColorProps)
      );
      let dynamicNumberOptions: ExtendedCommonStyleOptions = resolveExtendedCommonStyleOptions(
        createCustomComponentAttribute('Tabs', 'tabsDynamicNumberColor', dynamicNumberColorProps,
          new CommonStyleExpressionResolverStub())
      );
      expect(validOptions.backgroundColor).assertEqual('#FF102030');
      expect(literalNumberOptions.backgroundColor).assertEqual('transparent');
      expect(dynamicNumberOptions.backgroundColor).assertEqual('transparent');
    });

    it('should_use_theme_component_background_default_when_common_background_color_is_invalid', 0, () => {
      let invalidColorStyles: Record<string, Object> = { 'backgroundColor': 'invalid-color' };
      let invalidColorProps: Record<string, Object> = { 'styles': invalidColorStyles as Object };
      let lightOptions: ExtendedCommonStyleOptions = resolveExtendedCommonStyleOptions(
        createCustomComponentAttribute('Select', 'selectInvalidBackgroundColor', invalidColorProps),
        '#0C000000'
      );
      let darkOptions: ExtendedCommonStyleOptions = resolveExtendedCommonStyleOptions(
        createCustomComponentAttribute('Select', 'selectInvalidBackgroundColor', invalidColorProps),
        '#19FFFFFF'
      );

      expect(lightOptions.backgroundColor).assertEqual('#0C000000');
      expect(darkOptions.backgroundColor).assertEqual('#19FFFFFF');
    });

    it('should_merge_extended_common_style_defaults_without_overriding_resolved_values', 0, () => {
      let options: ExtendedCommonStyleOptions = mergeExtendedCommonStyleOptions(
        {
          width: '100%',
          height: '100%',
          backgroundColor: '#FFFFFFFF',
          layoutWeight: 0
        },
        {
          width: 20,
          backgroundColor: '#FF112233'
        }
      );

      expect(options.width).assertEqual(20);
      expect(options.height).assertEqual('100%');
      expect(options.backgroundColor).assertEqual('#FF112233');
      expect(options.layoutWeight).assertEqual(0);
    });

    it('should_preserve_previous_common_style_only_when_dynamic_resolution_fails', 0, () => {
      let options: ExtendedCommonStyleOptions = mergeExtendedCommonStyleOptions(
        { width: '100%', height: '100%' },
        { height: 80 },
        { width: 200, height: 60 },
        ['width']
      );

      expect(options.width).assertEqual(200);
      expect(options.height).assertEqual(80);
    });

    xit('should_use_extended_row_schema_provider_when_custom_row_catalog_item_is_created', 0, () => {
      let schema: string = getCustomComponentSchema(createExtendedRowDefinition());

      expect(schema.length > 0).assertTrue();
      expect(SchemaValidator.validateComponentSchema('Row', schema).length).assertEqual(0);
      expect(schema.indexOf('"itemMargin"') >= 0).assertTrue();
      expect(schema.indexOf('"justifyContent"') >= 0).assertTrue();
      expect(schema.indexOf('"alignItems"') >= 0).assertTrue();
      expect(schema.indexOf('"wrap"') >= 0).assertTrue();
      expect(schema.indexOf('ExtendedDynamicValueRef') >= 0).assertTrue();
      expect(schema.indexOf('"ExpressionString"')).assertEqual(-1);
      expect(schema.indexOf('ExtendedCommonStyles') >= 0).assertTrue();
      expect(schema.indexOf('"justify"')).assertEqual(-1);
      expect(schema.indexOf('"align"')).assertEqual(-1);
    });

    xit('should_expose_extended_select_schema_with_shared_common_styles_when_requested', 0, () => {
      let catalog: CatalogImpl = CatalogFactoryCore.extended();
      let schema: string = getComponentSchema(catalog, 'Select');

      expect(schema.length > 0).assertTrue();
      expect(SchemaValidator.validateComponentSchema('Select', schema).length).assertEqual(0);
      expect(schema.indexOf('"DynamicSelectStyles"') >= 0).assertTrue();
      expect(schema.indexOf('"divider"') >= 0).assertTrue();
      expect(schema.indexOf('ExtendedCommonStyles') >= 0).assertTrue();
    });

    it('should_not_normalize_extended_text_fields_in_ets_schema_validation', 0, () => {
      let catalog: CatalogImpl = CatalogFactoryCore.extended();
      const dsl =
        '{"version":"v0.9","updateComponents":{"surfaceId":"main","components":[{"id":"title",' +
        '"component":"Text","content":{},"styles":{"fontScaleMode":"unsupported","minFontScale":"bad",' +
        '"maxFontScale":true,"decoration":"broken"}}]}}';
      let result = SchemaValidator.validateAndNormalizeDsl(dsl, catalog);

      expect(result.success).assertTrue();
      let normalizedRoot = JSON.parse(result.normalizedDsl) as Record<string, Object>;
      let updateComponents = normalizedRoot['updateComponents'] as Record<string, Object>;
      let components = updateComponents['components'] as Object[];
      let text = components[0] as Record<string, Object>;
      let styles = text['styles'] as Record<string, Object>;

      expect(JSON.stringify(text['content'])).assertEqual('{}');
      expect(styles['fontScaleMode']).assertEqual('unsupported');
      expect(styles['minFontScale']).assertEqual('bad');
      expect(styles['maxFontScale']).assertEqual(true);
      expect(styles['decoration']).assertEqual('broken');
      expect(SchemaErrorInfoManager.getInstance().getWarningCount()).assertEqual(0);
    });

    it('should_keep_extended_text_dynamic_descriptors_without_schema_warnings', 0, () => {
      let catalog: CatalogImpl = CatalogFactoryCore.extended();
      const dsl =
        '{"version":"v0.9","updateComponents":{"surfaceId":"main","components":[{"id":"title",' +
        '"component":"Text","content":{"path":"/data/title"},"styles":{"fontScaleMode":{"path":' +
        '"/data/fontScaleMode"},"minFontScale":{"path":"/data/minFontScale"}}}]}}';
      let result = SchemaValidator.validateAndNormalizeDsl(dsl, catalog);

      expect(result.success).assertTrue();
      let normalizedRoot = JSON.parse(result.normalizedDsl) as Record<string, Object>;
      let updateComponents = normalizedRoot['updateComponents'] as Record<string, Object>;
      let components = updateComponents['components'] as Object[];
      let text = components[0] as Record<string, Object>;
      let styles = text['styles'] as Record<string, Object>;

      expect(JSON.stringify(text['content'])).assertEqual('{"path":"/data/title"}');
      expect(JSON.stringify(styles['fontScaleMode'])).assertEqual('{"path":"/data/fontScaleMode"}');
      expect(JSON.stringify(styles['minFontScale'])).assertEqual('{"path":"/data/minFontScale"}');
      expect(SchemaErrorInfoManager.getInstance().getWarningCount()).assertEqual(0);
    });

    it('should_keep_extended_text_decoration_member_bindings_without_schema_warnings', 0, () => {
      let catalog: CatalogImpl = CatalogFactoryCore.extended();
      const dsl =
        '{"version":"v0.9","updateComponents":{"surfaceId":"main","components":[{"id":"title",' +
        '"component":"Text","content":"title","styles":{"decoration":{"type":{"path":' +
        '"/data/decorationType"},"color":{"path":"/data/decorationColor"},"style":{"path":' +
        '"/data/decorationStyle"},"thicknessScale":{"path":"/data/decorationThickness"}}}}]}}';
      let result = SchemaValidator.validateAndNormalizeDsl(dsl, catalog);

      expect(result.success).assertTrue();
      let normalizedRoot = JSON.parse(result.normalizedDsl) as Record<string, Object>;
      let updateComponents = normalizedRoot['updateComponents'] as Record<string, Object>;
      let components = updateComponents['components'] as Object[];
      let text = components[0] as Record<string, Object>;
      let styles = text['styles'] as Record<string, Object>;
      let decoration = styles['decoration'] as Record<string, Object>;

      expect(JSON.stringify(decoration['type'])).assertEqual('{"path":"/data/decorationType"}');
      expect(JSON.stringify(decoration['color'])).assertEqual('{"path":"/data/decorationColor"}');
      expect(JSON.stringify(decoration['style'])).assertEqual('{"path":"/data/decorationStyle"}');
      expect(JSON.stringify(decoration['thicknessScale'])).assertEqual('{"path":"/data/decorationThickness"}');
      expect(SchemaErrorInfoManager.getInstance().getWarningCount()).assertEqual(0);
    });

    it('should_keep_extended_image_dynamic_src_without_schema_warnings', 0, () => {
      let catalog: CatalogImpl = CatalogFactoryCore.extended();
      const dsl =
        '{"version":"v0.9","updateComponents":{"surfaceId":"main","components":[{"id":"cover",' +
        '"component":"Image","src":{"path":"/data/imageSrc"}},{"id":"preview","component":"Image",' +
        '"src":"{{ \'/data/previewSrc\' }}"}]}}';
      let result = SchemaValidator.validateAndNormalizeDsl(dsl, catalog);

      expect(result.success).assertTrue();
      let normalizedRoot = JSON.parse(result.normalizedDsl) as Record<string, Object>;
      let updateComponents = normalizedRoot['updateComponents'] as Record<string, Object>;
      let components = updateComponents['components'] as Object[];
      let cover = components[0] as Record<string, Object>;
      let preview = components[1] as Record<string, Object>;

      expect(JSON.stringify(cover['src'])).assertEqual('{"path":"/data/imageSrc"}');
      expect(preview['src']).assertEqual('{{ \'/data/previewSrc\' }}');
      expect(SchemaErrorInfoManager.getInstance().getWarningCount()).assertEqual(0);
    });
  });
}
