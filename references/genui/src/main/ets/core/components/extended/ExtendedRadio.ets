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
import { CustomComponentDefinition } from '../A2UI/CustomComponentFactory';
import {
  parseCustomProps,
  resolveDynamicBooleanOptionValue,
  resolveDynamicStringOptionValue,
  dispatchComponentEvent
} from '../A2UI/CustomComponentUtils';
import { SchemaResourceLoader } from '../../schema/SchemaResourceLoader';
import { SchemaProvider } from '../../types/types';

const EXTENDED_RADIO_COMPONENT_TYPE: string = 'Radio';
const DEFAULT_CHECKED_BACKGROUND_COLOR: string = '#FF0A59F7';
const DEFAULT_UNCHECKED_BORDER_COLOR: string = '#33FFFFFF';
const DEFAULT_INDICATOR_COLOR: string = '#FFFFFFFF';

interface ExtendedRadioResolvedStyles {
  checkedBackgroundColor: string;
  unCheckedBorderColor: string;
  indicatorColor: string;
}

interface ExtendedRadioClickPayload {
  x: number;
  y: number;
}

@Component
export struct ExtendedRadio {
  private readonly tag: string = 'ExtendedRadio';
  @Prop @Watch('onAttributeChange') attribute: CustomComponentAttribute | undefined = undefined;
  @State radioChecked: boolean = false;
  @State radioValue: string = '';
  @State radioGroup: string = '';
  @State radioIndicatorType: RadioIndicatorType = RadioIndicatorType.TICK;
  @State checkedBackgroundColor: string = DEFAULT_CHECKED_BACKGROUND_COLOR;
  @State unCheckedBorderColor: string = DEFAULT_UNCHECKED_BORDER_COLOR;
  @State indicatorColor: string = DEFAULT_INDICATOR_COLOR;

  aboutToAppear(): void {
    this.syncState();
  }

  onAttributeChange(): void {
    this.syncState();
  }

  build() {
    Column() {
      Radio({ value: this.radioValue, group: this.radioGroup, indicatorType: this.radioIndicatorType })
        .checked(this.radioChecked)
        .radioStyle(this.buildRadioStyle())
        .onChange((isChecked: boolean) => {
          this.radioChecked = isChecked;
          this.dispatchChange(isChecked);
        })
    }
    .gesture(
      TapGesture()
        .onAction((event: GestureEvent) => {
          this.dispatchClick({
            x: event.offsetX,
            y: event.offsetY
          });
        })
    )
    .width('100%')
    .height('100%')
    .onAppear(() => {
      dispatchComponentEvent(this.attribute, 'onAppear');
    })
  }

  private syncState(): void {
    let customProps = parseCustomProps(this.attribute, this.tag);
    this.radioValue = this.resolveString(customProps, 'value', '');
    this.radioChecked = this.resolveBoolean(customProps, 'checked', false);
    this.radioGroup = this.resolveString(customProps, 'group', '');
    this.radioIndicatorType = this.resolveIndicatorType(customProps);

    let styles = this.resolveStyles(customProps);
    this.checkedBackgroundColor = styles.checkedBackgroundColor;
    this.unCheckedBorderColor = styles.unCheckedBorderColor;
    this.indicatorColor = styles.indicatorColor;
  }

  private buildRadioStyle(): RadioStyle {
    let radioStyle: RadioStyle = {
      checkedBackgroundColor: this.checkedBackgroundColor,
      uncheckedBorderColor: this.unCheckedBorderColor,
      indicatorColor: this.indicatorColor
    };
    return radioStyle;
  }

  private resolveIndicatorType(customProps: Record<string, Object> | undefined): RadioIndicatorType {
    if (customProps === undefined) {
      return RadioIndicatorType.TICK;
    }
    let resolvedValue: string | undefined = resolveDynamicStringOptionValue(customProps['indicatorType'], this.attribute);
    if (resolvedValue === undefined) {
      return RadioIndicatorType.TICK;
    }
    let value = resolvedValue.trim().toLowerCase();
    if (value === 'dot') {
      return RadioIndicatorType.DOT;
    }
    return RadioIndicatorType.TICK;
  }

  private resolveString(
    customProps: Record<string, Object> | undefined,
    key: string,
    fallback: string
  ): string {
    if (customProps === undefined) {
      return fallback;
    }
    let resolved: string | undefined = resolveDynamicStringOptionValue(customProps[key], this.attribute);
    return resolved !== undefined ? resolved : fallback;
  }

  private resolveBoolean(
    customProps: Record<string, Object> | undefined,
    key: string,
    fallback: boolean
  ): boolean {
    if (customProps === undefined) {
      return fallback;
    }
    let resolved: boolean | undefined = resolveDynamicBooleanOptionValue(customProps[key], this.attribute);
    return resolved !== undefined ? resolved : fallback;
  }

  private resolveStyles(customProps: Record<string, Object> | undefined): ExtendedRadioResolvedStyles {
    let result: ExtendedRadioResolvedStyles = {
      checkedBackgroundColor: DEFAULT_CHECKED_BACKGROUND_COLOR,
      unCheckedBorderColor: DEFAULT_UNCHECKED_BORDER_COLOR,
      indicatorColor: DEFAULT_INDICATOR_COLOR
    };

    if (customProps === undefined) {
      return result;
    }

    let styles = customProps['styles'];
    if (styles === undefined || typeof styles !== 'object' || styles === null || Array.isArray(styles)) {
      return result;
    }

    let styleRecord = styles as Record<string, Object>;
    let resolved: string | undefined;

    resolved = resolveDynamicStringOptionValue(styleRecord['checkedBackgroundColor'], this.attribute);
    if (resolved !== undefined) {
      result.checkedBackgroundColor = resolved;
    }

    resolved = resolveDynamicStringOptionValue(styleRecord['unCheckedBorderColor'], this.attribute);
    if (resolved !== undefined) {
      result.unCheckedBorderColor = resolved;
    }

    resolved = resolveDynamicStringOptionValue(styleRecord['indicatorColor'], this.attribute);
    if (resolved !== undefined) {
      result.indicatorColor = resolved;
    }

    return result;
  }

  private dispatchChange(checked: boolean): void {
    dispatchComponentEvent(this.attribute, 'onChange', { 'checked': checked as Object });
  }

  private dispatchClick(payload: ExtendedRadioClickPayload): void {
    dispatchComponentEvent(this.attribute, 'onClick', payload as Object as Record<string, Object>);
  }
}

@Builder
export function extendedRadioBuilder(attribute: CustomComponentAttribute): void {
  ExtendedRadio({ attribute: attribute });
}

export function createRadioDefinition(): CustomComponentDefinition {
  const schemaProvider: SchemaProvider = (_version: string): string => {
    return SchemaResourceLoader.loadSchema('schema/Extended/components/ExtendedRadio.json');
  };

  return {
    type: EXTENDED_RADIO_COMPONENT_TYPE,
    builder: wrapBuilder(extendedRadioBuilder),
    schemaProvider: schemaProvider
  };
}
