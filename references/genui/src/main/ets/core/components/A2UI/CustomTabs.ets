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

import { CustomComponentAttribute, resolveComponentBrandColor } from './CustomComponent';
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
import { A2UIDynamicString } from '../../types/DynamicString';
import { DynamicValueResolver } from '../../types/runtime/DynamicValueResolver';
import { A2UIResolveContext } from '../../types/runtime/ResolveContext';
import { SchemaLocalPropertyHelper } from '../../schema/SchemaLocalPropertyHelper';
import { NodeContent } from '@kit.ArkUI';

interface TabItem {
  title: string;
  child: string;
  slot?: NodeContent;
}

interface NormalizedTabDefinition {
  titleValue: A2UIDynamicString;
  child: string;
  slotKey: string;
}

const TABS_UNSELECTED_TEXT_COLOR_RESOURCE: Resource = $r('app.color.custom_text_primary');
const TABS_UNSELECTED_INDICATOR_COLOR: number = 0x00000000;
const TABS_COMPONENT_NAME: string = 'Tabs';
const TABS_PROPERTY_NAME: string = 'tabs';
const TABS_MIN_ITEMS: number = 1;
const TAB_TITLE_PROPERTY_NAME: string = 'title';
const TAB_CHILD_PROPERTY_NAME: string = 'child';
const TABS_ALLOWED_PROPERTY_NAMES: string[] = [TABS_PROPERTY_NAME];
const TAB_ITEM_ALLOWED_PROPERTY_NAMES: string[] = [TAB_TITLE_PROPERTY_NAME, TAB_CHILD_PROPERTY_NAME];

export function resolveTabsSelectedTextColor(
  isSelected: boolean,
  hasSelectedColor: boolean,
  selectedColor: ResourceColor
): ResourceColor {
  if (isSelected && hasSelectedColor) {
    return selectedColor;
  }
  return TABS_UNSELECTED_TEXT_COLOR_RESOURCE;
}

export function resolveTabsSelectedIndicatorColor(
  isSelected: boolean,
  hasSelectedColor: boolean,
  selectedColor: ResourceColor
): ResourceColor {
  if (isSelected && hasSelectedColor) {
    return selectedColor;
  }
  return TABS_UNSELECTED_INDICATOR_COLOR;
}

@Component
export struct CustomTabs {
  @Prop @Watch('onAttributeChange') attribute: CustomComponentAttribute | undefined = undefined;
  @State currentIndex: number = 0;
  @State tabItems: TabItem[] = [];
  private normalizedTabs: NormalizedTabDefinition[] = [];
  private hasNormalizedTabs: boolean = false;
  private lastNormalizedSignature: string = '';
  private lastUnknownFieldSignature: string = '';

  aboutToAppear(): void {
    this.synchronizeTabs();
  }

  onAttributeChange(): void {
    this.synchronizeTabs();
  }

  build() {
    Tabs({ index: this.currentIndex }) {
      if (this.hasBrandColor()) {
        ForEach(this.tabItems, (tab: TabItem) => {
          TabContent() {
            if (tab.child.length > 0 && tab.slot !== undefined) {
              ContentSlot(tab.slot)
            } else if (tab.child.length > 0) {
              Text('Loading...')
                .fontSize(16)
                .padding(20)
            }
          }
          .align(Alignment.TopStart)
          .tabBar(SubTabBarStyle.of(tab.title)
            .indicator({
              color: resolveTabsSelectedIndicatorColor(true, true, this.resolveBrandColor())
            })
            .labelStyle({
              selectedColor: resolveTabsSelectedTextColor(true, true, this.resolveBrandColor())
            }))
        })
      } else {
        ForEach(this.tabItems, (tab: TabItem) => {
          TabContent() {
            if (tab.child.length > 0 && tab.slot !== undefined) {
              ContentSlot(tab.slot)
            } else if (tab.child.length > 0) {
              Text('Loading...')
                .fontSize(16)
                .padding(20)
            }
          }
          .align(Alignment.TopStart)
          .tabBar(tab.title)
        })
      }
    }
    .height('auto')
    .barMode(BarMode.Fixed)
    .animationDuration(200)
    .onChange((index: number) => {
      this.currentIndex = index;
    })
    .layoutWeight(this.resolveWeight())
    .margin(this.resolveMargin())
    .accessibilityText(this.resolveAccessibilityLabel())
    .accessibilityDescription(this.resolveAccessibilityDescription())
  }

  private hasBrandColor(): boolean {
    return resolveComponentBrandColor(this.attribute) !== undefined;
  }

  private resolveBrandColor(): ResourceColor {
    return resolveComponentBrandColor(this.attribute) ?? 0;
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

  private synchronizeTabs(): void {
    this.ensureNormalizedTabsSynchronized();
    this.tabItems = this.resolveTabItems();
  }

  private ensureNormalizedTabsSynchronized(): void {
    let currentSignature: string = buildCustomPropsSignature(this.attribute);
    if (this.hasNormalizedTabs && this.lastNormalizedSignature === currentSignature) {
      return;
    }
    this.normalizedTabs = this.normalizeTabsDefinitions();
    this.hasNormalizedTabs = true;
    this.lastNormalizedSignature = currentSignature;
  }

  private normalizeTabsDefinitions(): NormalizedTabDefinition[] {
    return withAttributeSchemaWarningContext(this.attribute, (): NormalizedTabDefinition[] => {
      let customProps = this.attribute?.customProps;

      if (customProps === undefined || customProps === null ||
        Array.isArray(customProps) || typeof customProps !== 'object') {
        return [];
      }

      let parsed: Record<string, Object> = customProps as Record<string, Object>;
      const warningPathPrefix: string = resolveCustomComponentWarningPathPrefix(this.attribute);

      try {
        let tabsArray: Record<string, Object>[] =
          SchemaLocalPropertyHelper.normalizeObjectArrayProperty(
            parsed,
            TABS_PROPERTY_NAME,
            TABS_COMPONENT_NAME,
            warningPathPrefix,
            true,
            TABS_MIN_ITEMS
          );
        this.recordUnknownFieldWarningsIfNeeded(parsed, tabsArray, warningPathPrefix);

        let definitions: NormalizedTabDefinition[] = [];
        for (let i = 0; i < tabsArray.length; i++) {
          let tabObj: Record<string, Object> = tabsArray[i];
          let tabPath: string = `${warningPathPrefix}.${TABS_PROPERTY_NAME}[${i}]`;

          let titleValue: A2UIDynamicString =
            SchemaLocalPropertyHelper.normalizeDynamicStringProperty(
              tabObj,
              TAB_TITLE_PROPERTY_NAME,
              '',
              TABS_COMPONENT_NAME,
              tabPath,
              true
            );
          let normalizedChild: string | undefined =
            SchemaLocalPropertyHelper.normalizeNonEmptyStringProperty(
              tabObj,
              TAB_CHILD_PROPERTY_NAME,
              TABS_COMPONENT_NAME,
              tabPath,
              true
            );
          let child: string = normalizedChild ?? '';
          definitions.push({
            titleValue: titleValue,
            child: child,
            slotKey: `tab-${i}`
          });
        }
        return definitions;
      } catch (error) {
        console.error(
          `CustomTabs resolveTabs parse failed: ${JSON.stringify(error)}, ` +
            `stack: ${(error as Error).stack || 'no stack'}`
        );
        return [];
      }
    });
  }

  private recordUnknownFieldWarnings(
    parsed: Record<string, Object>,
    tabsArray: Record<string, Object>[],
    warningPathPrefix: string
  ): void {
    SchemaLocalPropertyHelper.recordUnknownFields(
      parsed,
      TABS_ALLOWED_PROPERTY_NAMES,
      TABS_COMPONENT_NAME,
      warningPathPrefix
    );
    for (let i = 0; i < tabsArray.length; i++) {
      SchemaLocalPropertyHelper.recordUnknownFields(
        tabsArray[i],
        TAB_ITEM_ALLOWED_PROPERTY_NAMES,
        TABS_COMPONENT_NAME,
        `${warningPathPrefix}.${TABS_PROPERTY_NAME}[${i}]`
      );
    }
  }

  private recordUnknownFieldWarningsIfNeeded(
    parsed: Record<string, Object>,
    tabsArray: Record<string, Object>[],
    warningPathPrefix: string
  ): void {
    let rootUnknownKeys: string[] = SchemaLocalPropertyHelper
      .collectUnknownFieldKeys(parsed, TABS_ALLOWED_PROPERTY_NAMES)
      .slice()
      .sort();
    let itemUnknownKeys: string[][] = tabsArray.map((tabItem: Record<string, Object>) => {
      return SchemaLocalPropertyHelper
        .collectUnknownFieldKeys(tabItem, TAB_ITEM_ALLOWED_PROPERTY_NAMES)
        .slice()
        .sort();
    });
    let currentUnknownFieldSignature: string = JSON.stringify({
      rootUnknownKeys: rootUnknownKeys,
      itemUnknownKeys: itemUnknownKeys
    });
    if (this.lastUnknownFieldSignature === currentUnknownFieldSignature) {
      return;
    }
    this.lastUnknownFieldSignature = currentUnknownFieldSignature;
    let hasUnknownFields: boolean = rootUnknownKeys.length > 0;
    if (!hasUnknownFields) {
      hasUnknownFields = itemUnknownKeys.some((keys: string[]) => keys.length > 0);
    }
    if (!hasUnknownFields) {
      return;
    }
    this.recordUnknownFieldWarnings(parsed, tabsArray, warningPathPrefix);
  }

  private resolveTabItems(): TabItem[] {
    let items: TabItem[] = [];
    let slots = this.attribute?.slots;
    for (let i = 0; i < this.normalizedTabs.length; i++) {
      let definition: NormalizedTabDefinition = this.normalizedTabs[i];
      let title: string;
      try {
        title = this.resolveDynamicString(definition.titleValue, definition.slotKey);
      } catch (e) {
        console.error(`CustomTabs: resolveDynamicString failed for ${definition.slotKey}: ${JSON.stringify(e)}`);
        title = String(definition.titleValue);
      }
      let tabSlot: NodeContent | undefined = undefined;
      try {
        if (slots instanceof Map) {
          tabSlot = slots.get(definition.slotKey);
        } else if (slots && typeof slots === 'object') {
          tabSlot = (slots as Record<string, NodeContent>)[definition.slotKey];
        }
      } catch (e) {
        console.error(`CustomTabs: failed to get tabSlot for ${definition.slotKey}: ${JSON.stringify(e)}`);
      }

      items.push({
        title: title,
        child: definition.child,
        slot: tabSlot
      });
    }
    return items;
  }

  private resolveDynamicString(value: A2UIDynamicString | undefined, context: string): string {
    let resolveContext: A2UIResolveContext | undefined = this.attribute?.dataModel !== undefined
      ? {
        renderId: this.attribute.renderId,
        dataModel: this.attribute.dataModel,
        surfaceId: this.attribute.surfaceId,
        componentId: this.attribute.id
      } as A2UIResolveContext
      : undefined;
    return DynamicValueResolver.resolveString(value, '', resolveContext);
  }
}

@Builder
export function customTabsBuilder(attribute: CustomComponentAttribute): void {
  CustomTabs({ attribute: attribute });
}

const tabsSchemaProvider: SchemaProvider = (version: string): string => {
  return SchemaResourceLoader.loadA2UISchema(version, 'components/Tabs.json');
};

export function createTabsDefinition(): CustomComponentDefinition {
  return {
    type: 'Tabs',
    builder: wrapBuilder(customTabsBuilder),
    schemaProvider: tabsSchemaProvider
  };
}

export function asCatalogItem(): CatalogItem {
  const definition: CustomComponentDefinition = createTabsDefinition();
  return CatalogItem.forComponent(
    definition.type,
    definition.schemaProvider ?? ((_version: string): string => ''),
    definition.builder
  ).markInnerNative(false);
}
