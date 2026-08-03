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

import { describe, it, expect } from '@ohos/hypium';
import { NodeContent } from '@kit.ArkUI';
import {
  CustomComponentAttribute,
  CustomComponentDescriptor,
  ComponentTheme,
  resolveComponentBrandColor
} from '../main/ets/core/components/A2UI/CustomComponent';
import {
  resolveChoicePickerChipBackgroundColor
} from '../main/ets/core/components/A2UI/CustomChoicePicker';
import {
  resolveDateTimeInputConfirmButtonBackgroundColor,
  resolveDateTimeInputDialogBackgroundColorByThemeMode
} from '../main/ets/core/components/A2UI/CustomDateTimeInput';
import {
  resolveTabsSelectedIndicatorColor,
  resolveTabsSelectedTextColor
} from '../main/ets/core/components/A2UI/CustomTabs';
import { resolveDividerColorByThemeMode } from '../main/ets/core/components/A2UI/CustomDivider';
import { buildCustomComponentAttribute } from '../main/ets/core/components/A2UI/CustomComponentFactory';
import { Breakpoint, ThemeMode } from '../main/ets/core/base/NativeEngineBridge';
import { CustomComponentDynamicValueResolver } from '../main/ets/core/types/runtime/CustomComponentDynamicValueResolver';
import { ChangeReason } from '../main/ets/core/types/types';

@Builder
function placeholderBuilder(_attribute: CustomComponentAttribute): void {}

function createThemeAttribute(
  type: string,
  builder: WrappedBuilder<[CustomComponentAttribute]>,
  componentTheme?: ComponentTheme
): CustomComponentAttribute {
  let resolvedTheme: ComponentTheme = componentTheme ?? (new Object() as ComponentTheme);
  return {
    type: type,
    id: '',
    surfaceId: 'theme-surface',
    protocolVersion: 'v0.9',
    catalogId: '',
    resolver: new CustomComponentDynamicValueResolver(),
    builder: builder,
    componentTheme: resolvedTheme,
    changeReason: ChangeReason.UPDATE_COMPONENT
  };
}

export default function ComponentThemeUnitTest() {
  describe('ComponentThemeTest', () => {
    const builder: WrappedBuilder<[CustomComponentAttribute]> = wrapBuilder(placeholderBuilder);

    it('should_copy_componentTheme_into_standard_custom_component_attribute', 0, () => {
      let descriptor: CustomComponentDescriptor = {
        type: 'Blank',
        id: 'blank-theme',
        surfaceId: 'surface-theme',
        renderId: 7,
        componentTheme: {
          primaryColor: '#FF6A00',
          brandColor: '#102030',
          darkPrimaryColor: '#CC5500',
          colorMode: ThemeMode.DARK,
          breakpoint: Breakpoint.LG,
          iconUrl: 'https://a2ui.test/icon.png',
          agentDisplayName: 'Theme Test Bot'
        }
      };

      let attribute: CustomComponentAttribute = buildCustomComponentAttribute(descriptor, {
        builder: builder
      });

      expect(attribute.componentTheme !== undefined).assertTrue();
      expect(attribute.componentTheme!.primaryColor).assertEqual('#FF6A00');
      expect(attribute.componentTheme!.brandColor).assertEqual('#102030');
      expect(attribute.componentTheme!.darkPrimaryColor).assertEqual('#CC5500');
      expect(attribute.componentTheme!.colorMode).assertEqual(ThemeMode.DARK);
      expect(attribute.componentTheme!.breakpoint).assertEqual(Breakpoint.LG);
      expect(attribute.componentTheme!.iconUrl).assertEqual('https://a2ui.test/icon.png');
      expect(attribute.componentTheme!.agentDisplayName).assertEqual('Theme Test Bot');
    });

    it('should_copy_componentTheme_into_tabs_custom_component_attribute', 0, () => {
      let descriptor: CustomComponentDescriptor = {
        type: 'Tabs',
        id: 'tabs-theme',
        surfaceId: 'surface-theme',
        renderId: 8,
        componentTheme: {
          primaryColor: '#0088FF',
          colorMode: ThemeMode.LIGHT,
          breakpoint: Breakpoint.SM,
          agentDisplayName: 'Theme B'
        }
      };

      let slots: Map<string, NodeContent> = new Map();
      let attribute: CustomComponentAttribute = buildCustomComponentAttribute(descriptor, {
        builder: builder,
        slots: slots
      });

      expect(attribute.slots !== undefined).assertTrue();
      expect(attribute.componentTheme !== undefined).assertTrue();
      expect(attribute.componentTheme!.primaryColor).assertEqual('#0088FF');
      expect(attribute.componentTheme!.colorMode).assertEqual(ThemeMode.LIGHT);
      expect(attribute.componentTheme!.breakpoint).assertEqual(Breakpoint.SM);
      expect(attribute.componentTheme!.agentDisplayName).assertEqual('Theme B');
    });

    it('should_prefer_resolved_brand_color_when_component_theme_contains_brand_color', 0, () => {
      let attribute: CustomComponentAttribute = createThemeAttribute('ChoicePicker', builder, {
        primaryColor: '#FF6A00',
        brandColor: '#112233'
      });

      expect(resolveComponentBrandColor(attribute)).assertEqual('#112233');
    });

    it('should_fallback_to_primary_color_when_component_theme_has_no_brand_color', 0, () => {
      let attribute: CustomComponentAttribute = createThemeAttribute('ChoicePicker', builder, {
        primaryColor: '#FF6A00'
      });

      expect(resolveComponentBrandColor(attribute)).assertEqual('#FF6A00');
    });

    it('should_not_resolve_component_brand_color_when_attribute_theme_is_absent', 0, () => {
      let attribute: CustomComponentAttribute = createThemeAttribute('DateTimeInput', builder);

      expect(resolveComponentBrandColor(attribute) === undefined).assertTrue();
      expect(resolveComponentBrandColor(undefined) === undefined).assertTrue();
    });

    it('should_use_brand_color_for_choice_picker_chip_background_only_when_theme_exists', 0, () => {
      let brandColor: string = '#FF6A00';
      let themedSelected = resolveChoicePickerChipBackgroundColor(true, true, brandColor, false);
      expect(themedSelected).assertEqual(brandColor);

      let unselectedWithTheme = resolveChoicePickerChipBackgroundColor(false, true, brandColor, false);
      let unselectedWithoutTheme = resolveChoicePickerChipBackgroundColor(false, false, '#000000', false);
      expect(unselectedWithTheme).assertEqual(unselectedWithoutTheme);

      let selectedWithoutTheme = resolveChoicePickerChipBackgroundColor(true, false, brandColor, false);
      expect(selectedWithoutTheme === brandColor).assertFalse();
    });

    it('should_use_brand_color_for_date_time_confirm_background_only_when_theme_exists', 0, () => {
      let brandColor: string = '#FF6A00';
      let themedBackground = resolveDateTimeInputConfirmButtonBackgroundColor(true, brandColor);
      expect(themedBackground).assertEqual(brandColor);

      let fallbackA = resolveDateTimeInputConfirmButtonBackgroundColor(false, brandColor);
      let fallbackB = resolveDateTimeInputConfirmButtonBackgroundColor(false, '#123456');
      expect(fallbackA).assertEqual(fallbackB);
      expect(fallbackA === brandColor).assertFalse();
    });

    it('should_resolve_date_time_dialog_background_by_theme_mode_with_light_fallback_when_mode_is_undefined', 0, () => {
      let lightColor = resolveDateTimeInputDialogBackgroundColorByThemeMode(ThemeMode.LIGHT);
      let darkColor = resolveDateTimeInputDialogBackgroundColorByThemeMode(ThemeMode.DARK);
      let fallbackColor = resolveDateTimeInputDialogBackgroundColorByThemeMode(undefined);

      expect(lightColor === darkColor).assertFalse();
      expect(fallbackColor === lightColor).assertTrue();
      expect(fallbackColor === darkColor).assertFalse();
    });

    it('should_use_brand_color_for_tabs_selected_text_and_indicator_only_when_theme_exists', 0, () => {
      let brandColor: string = '#FF6A00';

      expect(resolveTabsSelectedTextColor(true, true, brandColor)).assertEqual(brandColor);
      expect(resolveTabsSelectedIndicatorColor(true, true, brandColor)).assertEqual(brandColor);

      let unselectedTextWithTheme = resolveTabsSelectedTextColor(false, true, brandColor);
      let unselectedTextWithoutTheme = resolveTabsSelectedTextColor(false, false, '#123456');
      expect(unselectedTextWithTheme).assertEqual(unselectedTextWithoutTheme);

      let selectedTextWithoutTheme = resolveTabsSelectedTextColor(true, false, brandColor);
      expect(selectedTextWithoutTheme === brandColor).assertFalse();

      let unselectedIndicatorWithTheme = resolveTabsSelectedIndicatorColor(false, true, brandColor);
      let unselectedIndicatorWithoutTheme = resolveTabsSelectedIndicatorColor(false, false, '#123456');
      expect(unselectedIndicatorWithTheme).assertEqual(unselectedIndicatorWithoutTheme);

      let selectedIndicatorWithoutTheme = resolveTabsSelectedIndicatorColor(true, false, brandColor);
      expect(selectedIndicatorWithoutTheme === brandColor).assertFalse();
    });

    it('should_keep_tabs_selected_style_fallback_when_theme_is_absent', 0, () => {
      let brandColor: string = '#0088FF';
      let selectedTextWithoutTheme = resolveTabsSelectedTextColor(true, false, brandColor);
      let selectedIndicatorWithoutTheme = resolveTabsSelectedIndicatorColor(true, false, brandColor);

      expect(selectedTextWithoutTheme === brandColor).assertFalse();
      expect(selectedIndicatorWithoutTheme === brandColor).assertFalse();
    });

    it('should_resolve_divider_color_by_theme_mode_with_light_fallback_when_mode_is_undefined', 0, () => {
      let lightColor = resolveDividerColorByThemeMode(ThemeMode.LIGHT);
      let darkColor = resolveDividerColorByThemeMode(ThemeMode.DARK);
      let fallbackColor = resolveDividerColorByThemeMode(undefined);

      expect(lightColor === darkColor).assertFalse();
      expect(fallbackColor === lightColor).assertTrue();
      expect(fallbackColor === darkColor).assertFalse();
    });
  });
}
