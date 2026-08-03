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
  parseCustomProps,
  resolveCustomComponentWarningPathPrefix,
  withAttributeSchemaWarningContext,
  resolveMargin,
  Margin
} from './CustomComponentUtils';
import { SchemaResourceLoader } from '../../schema/SchemaResourceLoader';
import { CatalogItem } from '../../base/CatalogItem';
import { SchemaLocalPropertyHelper } from '../../schema/SchemaLocalPropertyHelper';
import { SchemaProvider } from '../../types/types';
import { ThemeMode } from '../../base/NativeEngineBridge';

export interface IconRenderState {
  renderPlaceholder: boolean;
  resolvedName: string;
  resourceName: string;
  isComposite: boolean;
  compositeKey: string;
}

const ICON_COMPONENT_TYPE: string = 'Icon';

const DEFAULT_ICON_NAME: string = 'help';
const DEFAULT_ICON_RESOURCE: string = 'sys.symbol.questionmark_circle';
const INVALID_ICON_PLACEHOLDER_NAME: string = 'grayPlaceholder';
const ICON_SIZE_RESOURCE: Resource = $r('app.float.custom_icon_size');
const ICON_PADDING_RESOURCE: Resource = $r('app.float.custom_icon_padding');
export const ICON_PLACEHOLDER_COLOR_RESOURCE: Resource = $r('app.color.custom_icon_placeholder');
export const ICON_DARK_PLACEHOLDER_COLOR_RESOURCE: Resource = $r('app.color.custom_icon_dark_placeholder');
const ICON_PLACEHOLDER_CORNER_RADIUS: number = 4;
const ICON_LIGHT_COLOR_RESOURCE: Resource = $r('app.color.custom_icon_light');
const ICON_DARK_COLOR_RESOURCE: Resource = $r('app.color.custom_icon_dark');

const ICON_RESOURCE_MAP: Map<string, string> = new Map<string, string>([
  ['accountCircle', 'sys.symbol.person_crop_circle_fill_1'],
  ['add', 'sys.symbol.plus'],
  ['arrowBack', 'sys.symbol.arrow_left'],
  ['arrowForward', 'sys.symbol.arrow_right'],
  ['attachFile', 'sys.symbol.paperclip'],
  ['calendarToday', 'sys.symbol.calendar'],
  ['call', 'sys.symbol.phone_fill'],
  ['camera', 'sys.symbol.camera'],
  ['check', 'sys.symbol.checkmark'],
  ['close', 'sys.symbol.xmark'],
  ['delete', 'sys.symbol.trash'],
  ['download', 'sys.media.arrow_down_circle'],
  ['edit', 'sys.symbol.square_and_pencil'],
  ['event', 'sys.symbol.calendar_badge_clock'],
  ['error', 'sys.symbol.exclamationmark_circle'],
  ['fastForward', 'sys.symbol.fast_forward'],
  ['favorite', 'sys.symbol.heart_fill'],
  ['favoriteOff', 'sys.symbol.heart'],
  ['folder', 'sys.symbol.folder'],
  ['help', DEFAULT_ICON_RESOURCE],
  ['home', 'sys.symbol.house'],
  ['info', 'sys.symbol.info_circle'],
  ['locationOn', 'sys.symbol.map_badge_local'],
  ['lock', 'sys.symbol.lock'],
  ['lockOpen', 'sys.symbol.lock_open'],
  ['mail', 'sys.symbol.envelope'],
  ['menu', 'sys.symbol.dot_grid_2x2'],
  ['moreVert', 'sys.symbol.dot_grid_1x2'],
  ['moreHoriz', 'sys.symbol.more'],
  ['notificationsOff', 'sys.symbol.bell_slash'],
  ['notifications', 'sys.symbol.bell_fill'],
  ['pause', 'sys.symbol.pause'],
  ['payment', 'sys.symbol.creditcard'],
  ['person', 'sys.symbol.person'],
  ['phone', 'sys.symbol.phone'],
  ['photo', 'sys.symbol.picture'],
  ['play', 'sys.symbol.play_fill'],
  ['print', 'sys.symbol.printer'],
  ['refresh', 'sys.symbol.arrow_counterclockwise'],
  ['rewind', 'sys.symbol.gobackward_15'],
  ['search', 'sys.symbol.magnifyingglass'],
  ['send', 'sys.symbol.paperplane_right_fill'],
  ['settings', 'sys.symbol.gearshape'],
  ['share', 'sys.symbol.share'],
  ['shoppingCart', 'sys.symbol.cart'],
  ['skipNext', 'sys.symbol.forward_end_fill'],
  ['skipPrevious', 'sys.symbol.backward_end_fill'],
  ['star', 'sys.symbol.star_fill'],
  ['starOff', 'sys.symbol.star'],
  ['stop', 'sys.symbol.stop_circle_fill'],
  ['upload', 'sys.symbol.arrowshape_up_to_line_fill'],
  ['visibility', 'sys.symbol.eye'],
  ['visibilityOff', 'sys.symbol.eye_slash'],
  ['volumeDown', 'sys.symbol.speaker'],
  ['volumeMute', 'sys.symbol.speaker_slash'],
  ['volumeOff', 'sys.symbol.speaker_wave_3_slash'],
  ['volumeUp', 'sys.symbol.speaker_wave_3'],
  ['warning', 'sys.symbol.exclamationmark_triangle_fill']
]);

const COMPOSITE_ICON_MAP: Map<string, string> = new Map<string, string>([
  ['starHalf', 'starHalf']
]);

function createIconAllowedNames(): string[] {
  let names: string[] = [];
  ICON_RESOURCE_MAP.forEach((_resource: string, name: string) => {
    names.push(name);
  });
  COMPOSITE_ICON_MAP.forEach((_value: string, name: string) => {
    names.push(name);
  });
  return names;
}

const ICON_ALLOWED_NAMES: string[] = createIconAllowedNames();

function createIconSystemSymbolNames(): string[] {
  let names: string[] = [];
  ICON_RESOURCE_MAP.forEach((_resource: string, name: string) => {
    names.push(name);
  });
  return names;
}

const ICON_SYSTEM_SYMBOL_NAMES: string[] = createIconSystemSymbolNames();

export function getIconSystemSymbolNames(): string[] {
  return ICON_SYSTEM_SYMBOL_NAMES.slice();
}

export function resolveIconSystemSymbolResourceName(name: string): string | undefined {
  return ICON_RESOURCE_MAP.get(name);
}

export function resolveIconDefaultColorByThemeMode(colorMode: ThemeMode | undefined): Resource {
  return colorMode === ThemeMode.DARK ? ICON_DARK_COLOR_RESOURCE : ICON_LIGHT_COLOR_RESOURCE;
}

export function resolveIconPlaceholderColorByThemeMode(colorMode: ThemeMode | undefined): Resource {
  return colorMode === ThemeMode.DARK ? ICON_DARK_PLACEHOLDER_COLOR_RESOURCE : ICON_PLACEHOLDER_COLOR_RESOURCE;
}

export function resolveIconRenderState(attribute: CustomComponentAttribute | undefined): IconRenderState {
  return withAttributeSchemaWarningContext(attribute, (): IconRenderState => {
    let parsed = parseCustomProps(attribute, 'CustomIcon');
    let source: Record<string, Object> = parsed ?? {} as Record<string, Object>;
    const warningPathPrefix: string = resolveCustomComponentWarningPathPrefix(attribute);
    if (parsed !== undefined) {
      SchemaLocalPropertyHelper.recordUnknownFields(parsed, ['name'], 'Icon', warningPathPrefix);
    }

    if (source['name'] === undefined) {
      SchemaLocalPropertyHelper.normalizeEnumStringProperty(
        source,
        'name',
        DEFAULT_ICON_NAME,
        ICON_ALLOWED_NAMES,
        'Icon',
        warningPathPrefix,
        true
      );
      return {
        renderPlaceholder: true,
        resolvedName: DEFAULT_ICON_NAME,
        resourceName: '',
        isComposite: false,
        compositeKey: ''
      };
    }

    let normalizedName = SchemaLocalPropertyHelper.normalizeEnumStringProperty(
      source,
      'name',
      DEFAULT_ICON_NAME,
      ICON_ALLOWED_NAMES,
      'Icon',
      warningPathPrefix,
      false,
      undefined,
      INVALID_ICON_PLACEHOLDER_NAME
    );
    if (normalizedName === INVALID_ICON_PLACEHOLDER_NAME) {
      return {
        renderPlaceholder: true,
        resolvedName: DEFAULT_ICON_NAME,
        resourceName: '',
        isComposite: false,
        compositeKey: ''
      };
    }

    let compositeKey = COMPOSITE_ICON_MAP.get(normalizedName);
    if (compositeKey !== undefined) {
      return {
        renderPlaceholder: false,
        resolvedName: normalizedName,
        resourceName: '',
        isComposite: true,
        compositeKey: compositeKey
      };
    }

    let resourceName = ICON_RESOURCE_MAP.get(normalizedName) ?? '';
    if (resourceName.length === 0) {
      return {
        renderPlaceholder: true,
        resolvedName: normalizedName,
        resourceName: '',
        isComposite: false,
        compositeKey: ''
      };
    }
    return {
      renderPlaceholder: false,
      resolvedName: normalizedName,
      resourceName: resourceName,
      isComposite: false,
      compositeKey: ''
    };
  });
}

@Component
export struct CustomIcon {
  @Prop attribute: CustomComponentAttribute | undefined = undefined;

  build() {
    this.IconRoot(
      resolveIconRenderState(this.attribute),
      this.resolveSize()
    );
  }

  @Builder
  private IconRoot(renderState: IconRenderState, iconSize: number | undefined) {
    Column() {
      if (renderState.renderPlaceholder) {
        this.buildPlaceholder(iconSize);
      } else if (renderState.isComposite) {
        this.buildComposite(renderState.compositeKey, iconSize);
      } else if (iconSize !== undefined) {
        SymbolGlyph(this.resolveResource(renderState.resourceName))
          .fontSize(iconSize)
          .fontColor([this.resolveColor()])
      } else {
        SymbolGlyph(this.resolveResource(renderState.resourceName))
          .fontSize(ICON_SIZE_RESOURCE)
          .fontColor([this.resolveColor()])
      }
    }
    .width(iconSize)
    .height(iconSize)
    .padding(iconSize !== undefined ? 0 : ICON_PADDING_RESOURCE)
    .justifyContent(FlexAlign.Center)
    .alignItems(HorizontalAlign.Center)
    .layoutWeight(this.resolveWeight())
    .margin(this.resolveMargin())
    .accessibilityText(this.resolveAccessibilityLabel())
    .accessibilityDescription(this.resolveAccessibilityDescription())
    .id(this.attribute?.id ?? '');
  }

  private resolveResource(resourceName: string): Resource {
    return $r(resourceName);
  }

  @Builder
  private buildPlaceholder(iconSize: number | undefined) {
    if (iconSize !== undefined) {
      Column()
        .width(iconSize)
        .height(iconSize)
        .backgroundColor(this.resolvePlaceholderColor())
        .borderRadius(ICON_PLACEHOLDER_CORNER_RADIUS);
    } else {
      Column()
        .width(ICON_SIZE_RESOURCE)
        .height(ICON_SIZE_RESOURCE)
        .backgroundColor(this.resolvePlaceholderColor())
        .borderRadius(ICON_PLACEHOLDER_CORNER_RADIUS);
    }
  }

  @Builder
  private buildComposite(compositeKey: string, iconSize: number | undefined) {
    if (compositeKey === 'starHalf') {
      this.buildHalfStar(iconSize);
    }
  }

  @Builder
  private buildHalfStar(iconSize: number | undefined) {
    if (iconSize !== undefined) {
      Stack() {
        SymbolGlyph($r('sys.symbol.star'))
          .fontSize(iconSize)
          .fontColor([this.resolveColor()])
        Column() {
          SymbolGlyph($r('sys.symbol.star_fill'))
            .fontSize(iconSize)
            .fontColor([this.resolveColor()])
        }
        .width('50%')
        .height('100%')
        .clip(true)
        .alignItems(HorizontalAlign.Start)
      }
      .width(iconSize)
      .height(iconSize)
      .alignContent(Alignment.TopStart)
    } else {
      Stack() {
        SymbolGlyph($r('sys.symbol.star'))
          .fontSize(ICON_SIZE_RESOURCE)
          .fontColor([this.resolveColor()])
        Column() {
          SymbolGlyph($r('sys.symbol.star_fill'))
            .fontSize(ICON_SIZE_RESOURCE)
            .fontColor([this.resolveColor()])
        }
        .width('50%')
        .height('100%')
        .clip(true)
        .alignItems(HorizontalAlign.Start)
      }
      .width(ICON_SIZE_RESOURCE)
      .height(ICON_SIZE_RESOURCE)
      .alignContent(Alignment.TopStart)
    }
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

  private resolveSize(): number | undefined {
    let customProps = parseCustomProps(this.attribute, 'CustomIcon');
    if (customProps === undefined) {
      return undefined;
    }
    if (customProps['size'] === undefined) {
      return undefined;
    }
    return customProps['size'] as number;
  }

  private resolveColor(): Resource | number {
    let customProps = parseCustomProps(this.attribute, 'CustomIcon');
    if (customProps === undefined) {
      return resolveIconDefaultColorByThemeMode(this.attribute?.componentTheme?.colorMode);
    }
    let colorValue = customProps['color'];
    if (typeof colorValue !== 'number') {
      return resolveIconDefaultColorByThemeMode(this.attribute?.componentTheme?.colorMode);
    }
    return colorValue;
  }

  private resolvePlaceholderColor(): Resource {
    return resolveIconPlaceholderColorByThemeMode(this.attribute?.componentTheme?.colorMode);
  }

  private resolveAccessibilityLabel(): string {
    return this.attribute?.properties?.accessibilityLabel ?? '';
  }

  private resolveAccessibilityDescription(): string {
    return this.attribute?.properties?.accessibilityDescription ?? '';
  }
}

@Builder
export function customIconBuilder(attribute: CustomComponentAttribute): void {
  CustomIcon({ attribute: attribute });
}

export function createIconDefinition(): CustomComponentDefinition {
  const schemaProvider: SchemaProvider = (version: string): string => {
    return SchemaResourceLoader.loadA2UISchema(version, 'components/Icon.json');
  };
  return {
    type: ICON_COMPONENT_TYPE,
    builder: wrapBuilder(customIconBuilder),
    schemaProvider: schemaProvider
  };
}

export function asCatalogItem(): CatalogItem {
  const definition: CustomComponentDefinition = createIconDefinition();
  return CatalogItem.forComponent(
    definition.type,
    definition.schemaProvider ?? ((_version: string): string => ''),
    definition.builder
  ).markInnerNative(false);
}
