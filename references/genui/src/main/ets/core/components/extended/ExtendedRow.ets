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

import { LengthMetrics, NodeContent } from '@kit.ArkUI';
import { CustomComponentAttribute } from '../A2UI/CustomComponent';
import { CustomComponentDefinition } from '../A2UI/CustomComponentFactory';
import {
  parseCustomProps,
  resolveDynamicNumberOptionValue,
  resolveDynamicStringOptionValue,
  dispatchComponentEvent
} from '../A2UI/CustomComponentUtils';
import { SchemaResourceLoader } from '../../schema/SchemaResourceLoader';
import { SchemaProvider } from '../../types/types';
import {
  applyExtendedCommonStyleOptions,
  ExtendedCommonStyleOptions,
  resolveExtendedCommonStyleOptions
} from './ExtendedCommonStyleModifier';

interface ExtendedRowResolvedChild {
  childId: string;
  slotKey: string;
  slot?: NodeContent;
}

const EXTENDED_ROW_COMPONENT_TYPE: string = 'Row';
const DEFAULT_ITEM_MARGIN: number = 16;
const MATCH_PARENT_PERCENT: Length = '100%';

class ExtendedRowStyleModifier implements AttributeModifier<FlexAttribute> {
  private readonly options: ExtendedCommonStyleOptions;

  constructor(attribute: CustomComponentAttribute | undefined) {
    this.options = resolveExtendedCommonStyleOptions(attribute);
    let styleProps = resolveExtendedRowStyleProps(parseCustomProps(attribute, EXTENDED_ROW_COMPONENT_TYPE));
    this.options.width = this.normalizeMatchParent(styleProps?.['width'], this.options.width);
    this.options.height = this.normalizeMatchParent(styleProps?.['height'], this.options.height);
  }

  applyNormalAttribute(instance: FlexAttribute): void {
    applyExtendedCommonStyleOptions(instance, this.options);
  }

  private normalizeMatchParent(
    rawValue: Object | undefined,
    fallbackValue: Length | LayoutPolicy | undefined
  ): Length | LayoutPolicy | undefined {
    if (isExtendedRowMatchParentValue(rawValue)) {
      return MATCH_PARENT_PERCENT;
    }
    return fallbackValue;
  }
}

function buildExtendedRowStyleModifier(attribute: CustomComponentAttribute | undefined): AttributeModifier<FlexAttribute> {
  return new ExtendedRowStyleModifier(attribute);
}

function resolveExtendedRowStyleProps(
  customProps: Record<string, Object> | undefined
): Record<string, Object> | undefined {
  let styleValue = customProps?.['styles'];
  if (typeof styleValue === 'object' && styleValue !== null && !Array.isArray(styleValue)) {
    return styleValue as Record<string, Object>;
  }
  return undefined;
}

export function resolveExtendedRowPrivateValue(
  customProps: Record<string, Object> | undefined,
  key: string
): Object | undefined {
  let styleProps = resolveExtendedRowStyleProps(customProps);
  if (styleProps !== undefined) {
    let styleValue = styleProps[key];
    if (styleValue !== undefined) {
      return styleValue;
    }
  }
  return customProps?.[key];
}

function isExtendedRowMatchParentValue(value: Object | undefined): boolean {
  if (typeof value === 'string') {
    let normalized = value.trim().toLowerCase();
    return normalized === 'matchparent' || normalized === 'match_parent';
  }
  return false;
}

@Component
export struct ExtendedRow {
  private readonly tag: string = 'ExtendedRow';
  @Prop @Watch('onAttributeChange') attribute: CustomComponentAttribute | undefined = undefined;

  @State rowItems: ExtendedRowResolvedChild[] = [];
  @State itemMargin: number = DEFAULT_ITEM_MARGIN;
  @State alignItems: ItemAlign = ItemAlign.Center;
  @State justifyContent: FlexAlign = FlexAlign.Start;
  @State wrap: FlexWrap = FlexWrap.NoWrap;

  aboutToAppear(): void {
    this.syncRowState();
  }

  onAttributeChange(): void {
    this.syncRowState();
  }

  build() {
    Flex({
      direction: FlexDirection.Row,
      wrap: this.wrap,
      justifyContent: this.justifyContent,
      alignItems: this.alignItems,
      space: {
        main: LengthMetrics.vp(this.itemMargin),
        cross: LengthMetrics.vp(this.itemMargin / 2)
      }
    }) {
      ForEach(this.rowItems, (item: ExtendedRowResolvedChild) => {
        if (item.slot !== undefined) {
          ContentSlot(item.slot)
        }
      }, (item: ExtendedRowResolvedChild) => `${item.slotKey}-${item.childId}`)
    }
    .layoutWeight(this.resolveWeight())
    .onClick(() => {
      dispatchComponentEvent(this.attribute, 'onClick');
    })
    .onAppear(() => {
      dispatchComponentEvent(this.attribute, 'onAppear');
    })
    .accessibilityText(this.resolveAccessibilityLabel())
    .accessibilityDescription(this.resolveAccessibilityDescription())
    .id(this.attribute?.id ?? '')
    .attributeModifier(buildExtendedRowStyleModifier(this.attribute))
  }

  private syncRowState(): void {
    let customProps = parseCustomProps(this.attribute, this.tag);
    let children = this.resolveChildren(customProps?.['children']);
    let resolvedJustifyContent = this.resolveJustifyContent(
      resolveExtendedRowPrivateValue(customProps, 'justifyContent')
    );
    let resolvedItemMargin = this.resolveItemMargin(customProps?.['itemMargin']);
    this.rowItems = this.resolveRowItems(children);
    this.alignItems = this.resolveAlignItems(resolveExtendedRowPrivateValue(customProps, 'alignItems'));
    this.justifyContent = resolvedJustifyContent;
    this.wrap = this.resolveWrap(resolveExtendedRowPrivateValue(customProps, 'wrap'));
    this.itemMargin = this.isItemMarginDisabledByJustify(resolvedJustifyContent) ? 0 : resolvedItemMargin;
  }

  private resolveChildren(value: Object | undefined): string[] {
    if (!Array.isArray(value)) {
      return [];
    }

    let children: string[] = [];
    let childEntries: Object[] = value as Object[];
    for (const entry of childEntries) {
      if (typeof entry === 'string' && entry.length > 0) {
        children.push(entry);
      }
    }
    return children;
  }

  private resolveRowItems(children: string[]): ExtendedRowResolvedChild[] {
    let items: ExtendedRowResolvedChild[] = [];
    for (let i = 0; i < children.length; i++) {
      let childId: string = children[i];
      let slotKey: string = `row-${i}`;
      items.push({
        childId: childId,
        slotKey: slotKey,
        slot: this.resolveSlotCandidate(slotKey, childId, i)
      });
    }
    return items;
  }

  private resolveSlotCandidate(slotKey: string, childId: string, index: number): NodeContent | undefined {
    let slotFromKey: NodeContent | undefined = this.resolveSlot(slotKey);
    if (slotFromKey !== undefined) {
      return slotFromKey;
    }

    if (childId.length > 0) {
      let slotFromChildId: NodeContent | undefined = this.resolveSlot(childId);
      if (slotFromChildId !== undefined) {
        return slotFromChildId;
      }
    }

    return this.resolveSlot(String(index));
  }

  private resolveSlot(slotKey: string): NodeContent | undefined {
    let slots = this.attribute?.slots;
    if (slots === undefined) {
      return undefined;
    }

    if (slots instanceof Map) {
      return slots.get(slotKey);
    }
    return (slots as Record<string, NodeContent>)[slotKey];
  }

  private resolveItemMargin(value: Object | undefined): number {
    let resolved: number | undefined = resolveDynamicNumberOptionValue(value, this.attribute);
    if (resolved !== undefined && resolved >= 0) {
      return resolved;
    }
    return DEFAULT_ITEM_MARGIN;
  }

  private resolveAlignItems(value: Object | undefined): ItemAlign {
    let resolved: string | undefined = resolveDynamicStringOptionValue(value, this.attribute);
    switch (resolved) {
      case 'top':
        return ItemAlign.Start;
      case 'bottom':
        return ItemAlign.End;
      case 'center':
      default:
        return ItemAlign.Center;
    }
  }

  private resolveJustifyContent(value: Object | undefined): FlexAlign {
    let resolved: string | undefined = resolveDynamicStringOptionValue(value, this.attribute);
    switch (resolved) {
      case 'center':
        return FlexAlign.Center;
      case 'end':
        return FlexAlign.End;
      case 'spaceAround':
        return FlexAlign.SpaceAround;
      case 'spaceBetween':
        return FlexAlign.SpaceBetween;
      case 'spaceEvenly':
        return FlexAlign.SpaceEvenly;
      case 'start':
      default:
        return FlexAlign.Start;
    }
  }

  private resolveWrap(value: Object | undefined): FlexWrap {
    let resolved: string | undefined = resolveDynamicStringOptionValue(value, this.attribute);
    switch (resolved) {
      case 'wrap':
        return FlexWrap.Wrap;
      case 'noWrap':
      default:
        return FlexWrap.NoWrap;
    }
  }

  private isItemMarginDisabledByJustify(justifyContent: FlexAlign): boolean {
    return justifyContent === FlexAlign.SpaceAround ||
      justifyContent === FlexAlign.SpaceBetween ||
      justifyContent === FlexAlign.SpaceEvenly;
  }

  private resolveLength(key: string, fallback: string | number): string | number {
    let customProps = parseCustomProps(this.attribute, this.tag);
    let value = resolveExtendedRowPrivateValue(customProps, key);
    if (typeof value === 'number' && Number.isFinite(value)) {
      return value;
    }
    if (typeof value === 'string' && value.length > 0) {
      return value;
    }
    return fallback;
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
}

@Builder
export function extendedRowBuilder(attribute: CustomComponentAttribute): void {
  ExtendedRow({ attribute: attribute });
}

export function createExtendedRowDefinition(): CustomComponentDefinition {
  const schemaProvider: SchemaProvider = (_version: string): string => {
    return SchemaResourceLoader.loadSchema('schema/Extended/components/ExtendedRow.json');
  };

  return {
    type: EXTENDED_ROW_COMPONENT_TYPE,
    builder: wrapBuilder(extendedRowBuilder),
    schemaProvider: schemaProvider
  };
}
