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

import { CustomComponentAttribute } from './CustomComponent';
import { CustomComponentDefinition } from './CustomComponentFactory';
import {
  resolveCustomComponentWarningPathPrefix,
  withAttributeSchemaWarningContext,
  resolveMargin,
  Margin,
  buildCustomPropsSignature
} from './CustomComponentUtils';
import { SchemaProvider } from '../../types/types';
import { SchemaResourceLoader } from '../../schema/SchemaResourceLoader';
import { CatalogItem } from '../../base/CatalogItem';
import { SchemaLocalPropertyHelper } from '../../schema/SchemaLocalPropertyHelper';
import { ThemeMode } from '../../base/NativeEngineBridge';

interface DividerOptions {
  axis: string;
}

const DIVIDER_LIGHT_COLOR: number = 0x33000000;
const DIVIDER_DARK_COLOR: number = 0x33FFFFFF;
const DIVIDER_HORIZONTAL_MARGIN_RESOURCE: Resource = $r('app.float.custom_divider_horizontal_margin');

export function resolveDividerColorByThemeMode(colorMode: ThemeMode | undefined): number {
  return colorMode === ThemeMode.DARK ? DIVIDER_DARK_COLOR : DIVIDER_LIGHT_COLOR;
}

@Component
export struct CustomDivider {
  @Prop attribute: CustomComponentAttribute | undefined = undefined;
  private resolvedOptions: DividerOptions | undefined = undefined;
  private lastResolvedSignature: string = '';

  aboutToAppear(): void {
    console.log("ArkTS CustomDivider start build");
  }

  build() {
    Column() {
      Divider()
      .vertical(this.resolveAxis() === 'vertical')
      .color(this.resolveColor())
      .margin({ left: DIVIDER_HORIZONTAL_MARGIN_RESOURCE, right: DIVIDER_HORIZONTAL_MARGIN_RESOURCE })
    }
    .layoutWeight(this.resolveWeight())
    .margin(this.resolveMargin())
    .accessibilityText(this.resolveAccessibilityLabel())
    .accessibilityDescription(this.resolveAccessibilityDescription())
    .id(this.attribute?.id ?? '')
  }

  private resolveColor(): number {
    return resolveDividerColorByThemeMode(this.attribute?.componentTheme?.colorMode);
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

  private resolveAxis(): string {
    return this.getResolvedOptions().axis;
  }

  private getResolvedOptions(): DividerOptions {
    let currentSignature: string = buildCustomPropsSignature(this.attribute);
    if (this.resolvedOptions !== undefined && this.lastResolvedSignature === currentSignature) {
      return this.resolvedOptions;
    }
    this.resolvedOptions = this.resolveOptionsForSchemaWarning();
    this.lastResolvedSignature = currentSignature;
    return this.resolvedOptions;
  }

  private resolveOptionsForSchemaWarning(): DividerOptions {
    return withAttributeSchemaWarningContext(this.attribute, (): DividerOptions => {
      let customProps = this.attribute?.customProps;
      if (customProps === undefined || customProps === null ||
        Array.isArray(customProps) || typeof customProps !== 'object') {
        return { axis: 'horizontal' };
      }

      let optionsMap: Record<string, Object> = customProps as Record<string, Object>;
      const warningPathPrefix: string = resolveCustomComponentWarningPathPrefix(this.attribute);
      SchemaLocalPropertyHelper.recordUnknownFields(optionsMap, ['axis'], 'Divider', warningPathPrefix);

      return {
        axis: SchemaLocalPropertyHelper.normalizeEnumStringProperty(
          optionsMap,
          'axis',
          'horizontal',
          ['horizontal', 'vertical'],
          'Divider',
          warningPathPrefix
        )
      };
    });
  }
}

@Builder
export function customDividerBuilder(attribute: CustomComponentAttribute): void {
  CustomDivider({ attribute: attribute });
}

const dividerSchemaProvider: SchemaProvider = (version: string): string => {
  return SchemaResourceLoader.loadA2UISchema(version, 'components/Divider.json');
};

export function createDividerDefinition(): CustomComponentDefinition {
  return {
    type: 'Divider',
    builder: wrapBuilder(customDividerBuilder),
    schemaProvider: dividerSchemaProvider
  };
}

export function asCatalogItem(): CatalogItem {
  const definition: CustomComponentDefinition = createDividerDefinition();
  return CatalogItem.forComponent(
    definition.type,
    definition.schemaProvider ?? ((_version: string): string => ''),
    definition.builder
  ).markInnerNative(false);
}
