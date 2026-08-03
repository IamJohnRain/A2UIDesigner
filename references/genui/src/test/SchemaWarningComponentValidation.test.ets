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

import { describe, beforeEach, afterEach, it, expect } from '@ohos/hypium';
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
import { SchemaResourceLoader } from '../main/ets/core/schema/SchemaResourceLoader';
import { SchemaValidator } from '../main/ets/core/schema/SchemaValidator';
import { SchemaLocalPropertyHelper } from '../main/ets/core/schema/SchemaLocalPropertyHelper';
import { createExtendedRowDefinition } from '../main/ets/core/components/extended/ExtendedRow';
import { CustomComponentDynamicValueResolver } from '../main/ets/core/types/runtime/CustomComponentDynamicValueResolver';
import {
  ExtendedCommonStyleOptions,
  resolveExtendedCommonStyleOptions
} from '../main/ets/core/components/extended/ExtendedCommonStyleModifier';

@Builder
function schemaWarningPlaceholderBuilder(_attribute: CustomComponentAttribute): void {}

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
  customProps?: Record<string, Object>
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
    resolver: new CustomComponentDynamicValueResolver(),
    builder: wrapBuilder(schemaWarningPlaceholderBuilder)
  };
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

    it('should_allow_extended_select_empty_optional_object_schema_values', 0, () => {
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
      expect(schema.indexOf('"content"') >= 0).assertTrue();
      expect(schema.indexOf('"styles"') >= 0).assertTrue();
      expect(schema.indexOf('"fontScaleMode"') >= 0).assertTrue();
      expect(schema.indexOf('"minFontScale"') >= 0).assertTrue();
      expect(schema.indexOf('"maxFontScale"') >= 0).assertTrue();
      expect(schema.indexOf('"decoration"') >= 0).assertTrue();
    });

    it('should_expose_extended_common_style_defs_for_shared_modifier_fields_when_requested', 0, () => {
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

    it('should_expose_extended_tabs_schema_with_literal_tab_index_and_on_change_wrapper_when_requested', 0, () => {
      let catalog: CatalogImpl = CatalogFactoryCore.extended();
      let schema: string = getComponentSchema(catalog, 'Tabs');

      expect(schema.length > 0).assertTrue();
      expect(SchemaValidator.validateComponentSchema('Tabs', schema).length).assertEqual(0);
      expect(schema.indexOf('"tabIndex"') >= 0).assertTrue();
      expect(schema.indexOf('"functionCall"') >= 0).assertTrue();
      expect(schema.indexOf('"enum": ["true", "false"]') >= 0).assertTrue();
      expect(schema.indexOf('"ExpressionString"') >= 0).assertTrue();
      expect(schema.indexOf('"DynamicBooleanLike"') >= 0).assertTrue();
      expect(schema.indexOf('"DynamicNumericLike"') >= 0).assertTrue();
      expect(schema.indexOf('"DynamicBarPosition"') >= 0).assertTrue();
      expect(schema.indexOf('ExtendedCommonStyles') >= 0).assertTrue();
    });

    it('should_expose_extended_tab_content_schema_with_protocol_styles_and_bindings_when_requested', 0, () => {
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
      expect(schema.indexOf('"pattern": "^-?(0|[1-9]\\\\d*)(?:\\\\.\\\\d+)?$"') >= 0).assertTrue();
      expect(schema.indexOf('"ExpressionString"') >= 0).assertTrue();
      expect(schema.indexOf('"DynamicTabType"') >= 0).assertTrue();
      expect(schema.indexOf('ExtendedCommonStyles') >= 0).assertTrue();
    });

    it('should_expose_extended_web_schema_with_url_and_size_styles_when_requested', 0, () => {
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

    it('should_use_extended_row_schema_provider_when_custom_row_catalog_item_is_created', 0, () => {
      let schema: string = getCustomComponentSchema(createExtendedRowDefinition());

      expect(schema.length > 0).assertTrue();
      expect(SchemaValidator.validateComponentSchema('Row', schema).length).assertEqual(0);
      expect(schema.indexOf('"itemMargin"') >= 0).assertTrue();
      expect(schema.indexOf('"justifyContent"') >= 0).assertTrue();
      expect(schema.indexOf('"alignItems"') >= 0).assertTrue();
      expect(schema.indexOf('"wrap"') >= 0).assertTrue();
      expect(schema.indexOf('"ExpressionString"') >= 0).assertTrue();
      expect(schema.indexOf('ExtendedCommonStyles') >= 0).assertTrue();
      expect(schema.indexOf('"justify"')).assertEqual(-1);
      expect(schema.indexOf('"align"')).assertEqual(-1);
    });

    it('should_expose_extended_select_schema_with_shared_common_styles_when_requested', 0, () => {
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
