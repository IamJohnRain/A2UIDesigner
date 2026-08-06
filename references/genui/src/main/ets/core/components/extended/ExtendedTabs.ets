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

import { NodeContent } from '@kit.ArkUI';
import { CustomComponentAttribute } from '../A2UI/CustomComponent';
import { CustomComponentDefinition } from '../A2UI/CustomComponentFactory';
import { A2UIValueType, SchemaProvider } from '../../types/types';
import { SchemaResourceLoader } from '../../schema/SchemaResourceLoader';
import {
  parseCustomProps,
  resolveDynamicBooleanOptionValue,
  resolveDynamicStringOptionValue,
  dispatchComponentEvent,
  resolveCustomComponentWarningPathPrefix
} from '../A2UI/CustomComponentUtils';
import { ExtendedTabsRegistry, ExtendedTabMetadata } from './ExtendedTabsRegistry';
import {
  buildExtendedCommonStyleModifier,
  ExtendedCommonStyleState
} from './ExtendedCommonStyleModifier';
import { ThemeMode } from '../../base/NativeEngineBridge';
import { A2UIFunctionCall, A2UIFunctionArgs, A2UIFunctionReturnType } from '../../types/FunctionCall';
import { A2UIResolveContext } from '../../types/runtime/ResolveContext';
import { A2UIDynamicValue } from '../../types/DynamicValue';
import { A2UIDynamicNumber } from '../../types/DynamicNumber';
import { DynamicValueResolver } from '../../types/runtime/DynamicValueResolver';
import { FunctionBridge } from '../../functions/FunctionBridge';
import { SchemaErrorCode, SchemaErrorInfoManager } from '../../schema/SchemaErrorInfoManager';

const EXTENDED_TABS_TYPE: string = 'Tabs';
const DEFAULT_SELECTED_COLOR_LIGHT: string = '#1F64FF';
const DEFAULT_SELECTED_COLOR_DARK: string = '#F4F8FF';
const DEFAULT_UNSELECTED_COLOR_LIGHT: string = '#182431';
const DEFAULT_UNSELECTED_COLOR_DARK: string = '#DADDE2';
const DEFAULT_TAB_BAR_BACKGROUND_COLOR_LIGHT: string = '#FFFFFF';
const DEFAULT_TAB_BAR_BACKGROUND_COLOR_DARK: string = '#000000';
const DEFAULT_TAB_FONT_WEIGHT: number = 500;
const DEFAULT_TAB_TYPE: string = '';
const DEFAULT_TAB_ICON_SIZE: number = 16;
const DEFAULT_TAB_SPACE: number = 0;
const DEFAULT_SELECTED_BACKGROUND_COLOR_LIGHT: string = '#00000000';
const DEFAULT_SELECTED_BACKGROUND_COLOR_DARK: string = '#00000000';
const DEFAULT_SELECTED_BORDER_COLOR_LIGHT: string = '#FF0A59F7';
const DEFAULT_SELECTED_BORDER_COLOR_DARK: string = '#FF317AF7';
const DEFAULT_UNSELECTED_BACKGROUND_COLOR: string = '#00000000';
const DEFAULT_UNSELECTED_BORDER_COLOR: string = '#00000000';
const CAPSULE_TAB_BORDER_RADIUS: number = 18;
const TABS_ON_CHANGE_PROP_NAME: string = 'onChange';

interface ExtendedTabsResolvedItem {
  childId: string;
  slotKey: string;
  fallbackTitle?: string;
  slot?: NodeContent;
}

interface ExtendedTabsResolvedChild {
  childId: string;
  slotKey: string;
  fallbackTitle?: string;
}

export interface ExtendedTabsIndexRangeResult {
  resolvedIndex: number;
  shouldReportWarning: boolean;
  warningMessage: string;
}

function mergeExtendedTabsMetadataWithFallback(
  metadata: ExtendedTabMetadata | undefined,
  fallbackTitle: string
): ExtendedTabMetadata {
  let fallback: ExtendedTabMetadata = {
    title: fallbackTitle,
    icon: '',
    selectedSrc: '',
    tabType: DEFAULT_TAB_TYPE
  };
  if (metadata === undefined) {
    return fallback;
  }
  return {
    title: metadata.title.length > 0 ? metadata.title : fallback.title,
    icon: metadata.icon,
    selectedSrc: metadata.selectedSrc,
    tabType: metadata.tabType.length > 0 ? metadata.tabType : fallback.tabType,
    selectedColor: metadata.selectedColor,
    unSelectedColor: metadata.unSelectedColor,
    defaultBackgroundColor: metadata.defaultBackgroundColor,
    selectedBackgroundColor: metadata.selectedBackgroundColor,
    defaultBorderColor: metadata.defaultBorderColor,
    selectedBorderColor: metadata.selectedBorderColor,
    fontSize: metadata.fontSize,
    fontWeight: metadata.fontWeight,
    iconSize: metadata.iconSize,
    space: metadata.space
  };
}

export function normalizeExtendedTabsIndexValue(value: number | undefined, fallback: number): number {
  if (value !== undefined && Number.isFinite(value) && Number.isInteger(value) && value >= 0) {
    return value;
  }
  return fallback;
}

export function resolveExtendedTabsBorderRadius(tabType: string): number {
  return tabType === 'capsule' ? CAPSULE_TAB_BORDER_RADIUS : 0;
}

export function buildExtendedTabsTemplateInstanceChildId(
  templateComponentId: string,
  templatePath: string,
  index: number
): string {
  return `${templatePath}${templateComponentId}:${index}:${templateComponentId}`;
}

export function normalizeExtendedTabsIndexForItemCount(
  value: number,
  itemCount: number
): ExtendedTabsIndexRangeResult {
  if (itemCount <= 0 || value < itemCount) {
    return {
      resolvedIndex: itemCount <= 0 ? 0 : value,
      shouldReportWarning: false,
      warningMessage: ''
    };
  }

  let fallbackIndex: number = itemCount - 1;
  return {
    resolvedIndex: fallbackIndex,
    shouldReportWarning: true,
    warningMessage: `Property tabIndex value ${value} is invalid. ` +
      `Valid values are from 0 to ${fallbackIndex}. Fallback to ${fallbackIndex}.`
  };
}

export function dispatchExtendedTabsIndexRangeWarning(
  attribute: CustomComponentAttribute | undefined,
  message: string
): void {
  let schemaErrorInfoManager: SchemaErrorInfoManager = SchemaErrorInfoManager.getInstance();
  let warningPath: string = `${resolveCustomComponentWarningPathPrefix(attribute)}.tabIndex`;
  schemaErrorInfoManager.withSchemaWarningContext<void>(
    attribute?.renderId,
    attribute?.surfaceId,
    (): void => {
      let captureResult = schemaErrorInfoManager.captureWarnings<void>(() => {
        schemaErrorInfoManager.recordSchemaWarning(
          SchemaErrorCode.INVALID_VALUE,
          message,
          warningPath,
          'component',
          EXTENDED_TABS_TYPE
        );
      });
      schemaErrorInfoManager.dispatchCapturedWarnings(attribute?.surfaceId, captureResult.warnings);
    }
  );
}

@Component
struct ExtendedTabsBarItem {
  @Link currentIndex: number;
  @Prop attribute: CustomComponentAttribute | undefined = undefined;
  @Prop index: number = 0;
  @Prop darkMode: boolean = false;
  @Prop title: string = '';
  @Prop icon: string = '';
  @Prop selectedSrc: string = '';
  @Prop tabType: string = DEFAULT_TAB_TYPE;
  @Prop selectedColor: string = DEFAULT_SELECTED_COLOR_LIGHT;
  @Prop unSelectedColor: string = DEFAULT_UNSELECTED_COLOR_LIGHT;
  @Prop defaultBackgroundColor: string = DEFAULT_UNSELECTED_BACKGROUND_COLOR;
  @Prop selectedBackgroundColor: string = DEFAULT_SELECTED_BACKGROUND_COLOR_LIGHT;
  @Prop defaultBorderColor: string = DEFAULT_UNSELECTED_BORDER_COLOR;
  @Prop selectedBorderColor: string = DEFAULT_SELECTED_BORDER_COLOR_LIGHT;
  @Prop fontSize: number = 16;
  @Prop fontWeight: number | string = DEFAULT_TAB_FONT_WEIGHT;
  @Prop iconSize: number = DEFAULT_TAB_ICON_SIZE;
  @Prop space: number = DEFAULT_TAB_SPACE;

  build() {
    Column() {
      Row({ space: this.space }) {
        if (this.resolveIconSource(this.isSelected()).length > 0) {
          Image(this.resolveIconSource(this.isSelected()))
            .width(this.iconSize)
            .height(this.iconSize)
        }
        Text(this.title)
          .fontSize(this.fontSize)
          .fontWeight(this.fontWeight)
          .fontColor(this.resolveTitleColor(this.isSelected()))
      }
      .padding({ left: 12, right: 12, top: 8, bottom: 8 })
      .constraintSize({ minHeight: 36 })
      .backgroundColor(this.resolveTitleBackgroundColor(this.isSelected()))
      .border(this.resolveTitleBorderOptions(this.isSelected()))
      .borderRadius(resolveExtendedTabsBorderRadius(this.tabType))

      if (this.shouldRenderUnderline()) {
        Row()
          .width(this.resolveUnderlineWidth())
          .height(2)
          .backgroundColor(this.isSelected() ? this.resolveTitleColor(true) : 'transparent')
      }
    }
    .alignItems(HorizontalAlign.Center)
    .onClick(() => {
      if (this.currentIndex !== this.index) {
        this.currentIndex = this.index;
      }
      dispatchComponentEvent(this.attribute, 'onClick');
    })
  }

  private isSelected(): boolean {
    return this.currentIndex === this.index;
  }

  private resolveIconSource(selected: boolean): string {
    if (selected && this.selectedSrc.length > 0) {
      return this.selectedSrc;
    }
    return this.icon;
  }

  private resolveTitleColor(selected: boolean): string {
    if (this.darkMode && this.isUnderlineType()) {
      return '#FFFFFF';
    }
    return selected ? this.selectedColor : this.unSelectedColor;
  }

  private isUnderlineType(): boolean {
    return this.tabType === 'underline';
  }

  private resolveTitleBackgroundColor(selected: boolean): ResourceColor {
    if (selected) {
      return this.selectedBackgroundColor ?? this.defaultBackgroundColor ?? 'transparent';
    }
    return this.defaultBackgroundColor ?? 'transparent';
  }

  private resolveTitleBorderOptions(selected: boolean): BorderOptions {
    let borderColor = this.resolveTitleBorderColor(selected);
    if (borderColor === undefined) {
      return {
        width: 0
      };
    }
    return {
      width: 1,
      color: borderColor
    };
  }

  private resolveTitleBorderColor(selected: boolean): ResourceColor | undefined {
    if (selected) {
      return this.selectedBorderColor ?? this.defaultBorderColor;
    }
    return this.defaultBorderColor;
  }

  private shouldRenderUnderline(): boolean {
    return this.isUnderlineType();
  }

  private resolveUnderlineWidth(): string {
    let title = this.title.trim();
    if (title.length === 0) {
      return '20vp';
    }

    let asciiCount = 0;
    let wideCount = 0;
    for (let i = 0; i < title.length; i++) {
      if (title.charCodeAt(i) <= 0x7f) {
        asciiCount++;
      } else {
        wideCount++;
      }
    }

    let estimatedWidth = asciiCount * this.fontSize * 0.55 + wideCount * this.fontSize * 0.95;
    if (estimatedWidth < 20) {
      estimatedWidth = 20;
    }
    if (estimatedWidth > 240) {
      estimatedWidth = 240;
    }
    return `${Math.round(estimatedWidth)}vp`;
  }
}

@Component
export struct ExtendedTabs {
  private readonly tag: string = 'ExtendedTabs';
  private commonStyleState: ExtendedCommonStyleState = {};
  @Prop @Watch('onAttributeChange') attribute: CustomComponentAttribute | undefined = undefined;
  @State @Watch('onCurrentIndexChange') currentIndex: number = 0;
  @State tabItems: ExtendedTabsResolvedItem[] = [];
  private metadataListener: (() => void) | undefined = undefined;
  private listenerOwnerKey: string = '';
  private lastBoundTabIndex: number | undefined = undefined;
  private knownTabContentChildIds: Set<string> = new Set();
  private enableOnChangeDispatch: boolean = false;
  private lastDispatchedIndex: number = -1;
  private lastInvalidTabIndexSignature: string = '';

  aboutToAppear(): void {
    this.listenerOwnerKey = this.resolveListenerOwnerKey();
    this.syncTabsState();
    this.lastDispatchedIndex = this.currentIndex;
    this.enableOnChangeDispatch = true;
    this.metadataListener = () => {
      this.syncTabsState();
    };
    ExtendedTabsRegistry.addListener(this.metadataListener, this.listenerOwnerKey);
  }

  aboutToDisappear(): void {
    if (this.metadataListener !== undefined) {
      ExtendedTabsRegistry.removeListener(this.metadataListener, this.listenerOwnerKey);
      this.metadataListener = undefined;
    }
    this.lastBoundTabIndex = undefined;
    this.knownTabContentChildIds.clear();
    this.enableOnChangeDispatch = false;
    this.lastDispatchedIndex = -1;
    this.lastInvalidTabIndexSignature = '';
  }

  onAttributeChange(): void {
    this.syncTabsState();
  }

  onCurrentIndexChange(): void {
    if (!this.enableOnChangeDispatch || this.tabItems.length === 0) {
      return;
    }
    if (this.lastDispatchedIndex === this.currentIndex) {
      return;
    }
    this.lastDispatchedIndex = this.currentIndex;
    this.dispatchOnChange(this.currentIndex);
  }

  build() {
    if (this.tabItems.length > 0) {
      Tabs({ index: this.currentIndex }) {
        ForEach(this.tabItems, (tab: ExtendedTabsResolvedItem, index: number) => {
          TabContent() {
            if (tab.slot !== undefined) {
              ContentSlot(tab.slot)
            }
          }
          .tabBar(this.buildTabBar(tab, index))
        })
      }
      .vertical(this.resolveVertical())
      .barPosition(this.resolveBarPosition())
      .scrollable(this.resolveScrollable())
      .barMode(BarMode.Fixed)
      .barBackgroundColor(this.resolveTabBarBackgroundColor())
      .animationDuration(200)
      .onChange((index: number) => {
        this.currentIndex = index;
        // Force tab bar rebuild for custom underline/capsule visuals.
        this.tabItems = this.tabItems.slice();
      })
      .onAppear(() => {
        dispatchComponentEvent(this.attribute, 'onAppear');
      })
      .accessibilityText(this.resolveAccessibilityLabel())
      .accessibilityDescription(this.resolveAccessibilityDescription())
      .id(this.attribute?.id ?? '')
      .attributeModifier(buildExtendedCommonStyleModifier<TabsAttribute>(
        this.attribute,
        {
          layoutWeight: this.resolveWeight()
        },
        this.commonStyleState
      ))
    }
  }

  @Builder
  private buildTabBar(tab: ExtendedTabsResolvedItem, index: number) {
    ExtendedTabsBarItem({
      currentIndex: $currentIndex,
      attribute: this.attribute,
      index: index,
      darkMode: this.isDarkMode(),
      title: this.resolveMetadata(tab, index).title,
      icon: this.resolveMetadata(tab, index).icon,
      selectedSrc: this.resolveMetadata(tab, index).selectedSrc,
      tabType: this.resolveMetadata(tab, index).tabType,
      selectedColor: this.resolveMetadata(tab, index).selectedColor ?? this.resolveDefaultSelectedColor(),
      unSelectedColor: this.resolveMetadata(tab, index).unSelectedColor ?? this.resolveDefaultUnselectedColor(),
      defaultBackgroundColor: this.resolveMetadata(tab, index).defaultBackgroundColor ??
        DEFAULT_UNSELECTED_BACKGROUND_COLOR,
      selectedBackgroundColor: this.resolveMetadata(tab, index).selectedBackgroundColor ??
        this.resolveDefaultSelectedBackgroundColor(),
      defaultBorderColor: this.resolveMetadata(tab, index).defaultBorderColor ?? DEFAULT_UNSELECTED_BORDER_COLOR,
      selectedBorderColor: this.resolveMetadata(tab, index).selectedBorderColor ??
        this.resolveDefaultSelectedBorderColor(),
      fontSize: this.resolveMetadata(tab, index).fontSize ?? 16,
      fontWeight: this.resolveMetadata(tab, index).fontWeight ?? DEFAULT_TAB_FONT_WEIGHT,
      iconSize: this.resolveMetadata(tab, index).iconSize ?? DEFAULT_TAB_ICON_SIZE,
      space: this.resolveMetadata(tab, index).space ?? DEFAULT_TAB_SPACE
    })
  }

  private resolveMetadata(tab: ExtendedTabsResolvedItem, index: number): ExtendedTabMetadata {
    let metadata = ExtendedTabsRegistry.getMetadata(this.resolveSurfaceId(), tab.childId);
    return mergeExtendedTabsMetadataWithFallback(metadata, this.resolveFallbackTitle(tab, index));
  }

  private isDarkMode(): boolean {
    return this.attribute?.componentTheme?.colorMode === ThemeMode.DARK;
  }

  private resolveDefaultSelectedColor(): string {
    return this.isDarkMode() ? DEFAULT_SELECTED_COLOR_DARK : DEFAULT_SELECTED_COLOR_LIGHT;
  }

  private resolveDefaultUnselectedColor(): string {
    return this.isDarkMode() ? DEFAULT_UNSELECTED_COLOR_DARK : DEFAULT_UNSELECTED_COLOR_LIGHT;
  }

  private resolveDefaultSelectedBackgroundColor(): string {
    return this.isDarkMode() ? DEFAULT_SELECTED_BACKGROUND_COLOR_DARK :
      DEFAULT_SELECTED_BACKGROUND_COLOR_LIGHT;
  }

  private resolveDefaultSelectedBorderColor(): string {
    return this.isDarkMode() ? DEFAULT_SELECTED_BORDER_COLOR_DARK : DEFAULT_SELECTED_BORDER_COLOR_LIGHT;
  }

  private syncTabsState(): void {
    let items: ExtendedTabsResolvedItem[] = this.resolveTabItems();
    this.tabItems = items;
    if (items.length === 0) {
      this.currentIndex = 0;
      this.lastBoundTabIndex = undefined;
      this.lastInvalidTabIndexSignature = '';
      return;
    }

    let customProps = parseCustomProps(this.attribute, this.tag);
    let hasExplicitTabIndex: boolean = customProps !== undefined && this.hasObjectKey(customProps, 'tabIndex');
    let targetIndex: number = this.resolveTabIndex(customProps);
    if (targetIndex < 0) {
      targetIndex = 0;
    }
    let rangeResult: ExtendedTabsIndexRangeResult =
      normalizeExtendedTabsIndexForItemCount(targetIndex, items.length);
    if (hasExplicitTabIndex && rangeResult.shouldReportWarning) {
      this.dispatchTabIndexRangeWarning(targetIndex, items.length, rangeResult.warningMessage);
    } else {
      this.lastInvalidTabIndexSignature = '';
    }
    targetIndex = rangeResult.resolvedIndex;
    // Keep user-selected tab when metadata updates but bound tabIndex has not changed.
    let shouldApplyBoundIndex: boolean = this.lastBoundTabIndex === undefined || targetIndex !== this.lastBoundTabIndex;
    if (shouldApplyBoundIndex) {
      this.currentIndex = targetIndex;
    } else {
      if (this.currentIndex < 0) {
        this.currentIndex = 0;
      } else if (this.currentIndex >= items.length) {
        this.currentIndex = items.length - 1;
      }
    }
    this.lastBoundTabIndex = targetIndex;
  }

  private resolveTabItems(): ExtendedTabsResolvedItem[] {
    let customProps = parseCustomProps(this.attribute, this.tag);
    let hasExplicitChildren: boolean = customProps !== undefined && this.hasObjectKey(customProps, 'children');
    let resolvedChildren: ExtendedTabsResolvedChild[] = this.resolveChildrenFromChildren(customProps);
    if (!hasExplicitChildren && resolvedChildren.length === 0) {
      resolvedChildren = this.resolveChildrenFromSlots();
    }
    if (!hasExplicitChildren) {
      resolvedChildren = this.filterNonTabContentChildren(resolvedChildren);
    }

    let items: ExtendedTabsResolvedItem[] = [];
    for (let i = 0; i < resolvedChildren.length; i++) {
      let child: ExtendedTabsResolvedChild = resolvedChildren[i];
      items.push({
        childId: child.childId,
        slotKey: child.slotKey,
        fallbackTitle: child.fallbackTitle,
        slot: this.resolveSlot(child.slotKey)
      });
    }
    return items;
  }

  private resolveChildrenFromChildren(
    customProps: Record<string, Object> | undefined
  ): ExtendedTabsResolvedChild[] {
    if (customProps === undefined) {
      return [];
    }

    let childrenValue = customProps['children'];
    if (Array.isArray(childrenValue)) {
      let children: ExtendedTabsResolvedChild[] = [];
      for (let i = 0; i < childrenValue.length; i++) {
        let childId: string = typeof childrenValue[i] === 'string' ? childrenValue[i] as string : '';
        if (childId.length === 0) {
          continue;
        }
        children.push({
          childId: childId,
          slotKey: `tab-${i}`
        });
      }
      return children;
    }

    if (typeof childrenValue !== 'object' || childrenValue === null || Array.isArray(childrenValue)) {
      return [];
    }

    let templateChildren = childrenValue as Record<string, Object>;
    let templateComponentId: string = typeof templateChildren['componentId'] === 'string'
      ? templateChildren['componentId'] as string
      : '';
    let templatePath: string = typeof templateChildren['path'] === 'string'
      ? templateChildren['path'] as string
      : '';
    if (templateComponentId.length === 0 || templatePath.length === 0) {
      return [];
    }

    let templateItems = this.resolveArrayByPath(templatePath);
    let children: ExtendedTabsResolvedChild[] = [];
    for (let i = 0; i < templateItems.length; i++) {
      children.push({
        childId: this.buildTemplateInstanceChildId(templateComponentId, templatePath, i),
        slotKey: `tab-${i}`,
        fallbackTitle: this.resolveTemplateItemTitle(templateItems[i])
      });
    }
    return children;
  }

  private resolveChildrenFromSlots(): ExtendedTabsResolvedChild[] {
    let keys: string[] = this.resolveTabSlotKeys();
    let children: ExtendedTabsResolvedChild[] = [];
    for (let i = 0; i < keys.length; i++) {
      children.push({
        childId: keys[i],
        slotKey: keys[i]
      });
    }
    return children;
  }

  private resolveTabSlotKeys(): string[] {
    let slots = this.attribute?.slots;
    if (slots === undefined) {
      return [];
    }

    let keys: string[] = [];
    let tabKeys: string[] = [];
    if (slots instanceof Map) {
      slots.forEach((_value: NodeContent, key: string) => {
        keys.push(key);
        if (key.startsWith('tab-')) {
          tabKeys.push(key);
        }
      });
    } else {
      let slotsObject = slots as Record<string, NodeContent>;
      Object.keys(slotsObject).forEach((key: string) => {
        keys.push(key);
        if (key.startsWith('tab-')) {
          tabKeys.push(key);
        }
      });
    }

    if (tabKeys.length > 0) {
      keys = tabKeys;
    }

    keys.sort((left: string, right: string) => {
      return this.resolveSlotIndex(left) - this.resolveSlotIndex(right);
    });
    return keys;
  }

  private resolveSlotIndex(slotKey: string): number {
    if (!slotKey.startsWith('tab-')) {
      return Number.MAX_SAFE_INTEGER;
    }
    let indexValue = Number(slotKey.slice(4));
    if (!Number.isFinite(indexValue)) {
      return Number.MAX_SAFE_INTEGER;
    }
    return Math.floor(indexValue);
  }

  private filterNonTabContentChildren(children: ExtendedTabsResolvedChild[]): ExtendedTabsResolvedChild[] {
    if (children.length === 0) {
      this.knownTabContentChildIds.clear();
      return children;
    }

    let filteredChildren: ExtendedTabsResolvedChild[] = [];
    let hasTemplateInstanceChild: boolean = this.hasTemplateInstanceChildren(children);
    for (const child of children) {
      if (ExtendedTabsRegistry.getMetadata(this.resolveSurfaceId(), child.childId) !== undefined) {
        this.knownTabContentChildIds.add(child.childId);
        filteredChildren.push(child);
        continue;
      }

      if (this.knownTabContentChildIds.has(child.childId)) {
        filteredChildren.push(child);
        continue;
      }

      if (this.resolveSlot(child.slotKey) !== undefined) {
        filteredChildren.push(child);
        continue;
      }

      if (hasTemplateInstanceChild && this.isTemplateInstanceChildId(child.childId)) {
        continue;
      }
    }

    if (filteredChildren.length === 0) {
      return children;
    }
    if (filteredChildren.length < children.length && hasTemplateInstanceChild) {
      return children;
    }
    return filteredChildren;
  }

  private hasTemplateInstanceChildren(children: ExtendedTabsResolvedChild[]): boolean {
    for (const child of children) {
      if (this.isTemplateInstanceChildId(child.childId)) {
        return true;
      }
    }
    return false;
  }

  private isTemplateInstanceChildId(childId: string): boolean {
    if (childId.indexOf(':idx:') >= 0) {
      return true;
    }
    let parts = childId.split(':');
    if (parts.length < 3) {
      return false;
    }
    return /^[0-9]+$/.test(parts[parts.length - 2]);
  }

  private buildTemplateInstanceChildId(templateComponentId: string, templatePath: string, index: number): string {
    return buildExtendedTabsTemplateInstanceChildId(templateComponentId, templatePath, index);
  }

  private resolveArrayByPath(path: string): Object[] {
    if (path.length === 0) {
      return [];
    }

    let currentValue: Object | undefined = this.attribute?.dataModel as Object | undefined;
    if (currentValue === undefined || currentValue === null) {
      return [];
    }

    let segments: string[] = path.split('/');
    for (const segment of segments) {
      if (segment.length === 0) {
        continue;
      }

      if (Array.isArray(currentValue)) {
        let index: number = Number(segment);
        if (!Number.isInteger(index) || index < 0 || index >= currentValue.length) {
          return [];
        }
        currentValue = currentValue[index] as Object;
        continue;
      }

      if (typeof currentValue !== 'object' || currentValue === null) {
        return [];
      }

      let currentObject = currentValue as Record<string, Object>;
      if (!this.hasObjectKey(currentObject, segment)) {
        return [];
      }
      currentValue = currentObject[segment];
    }

    if (!Array.isArray(currentValue)) {
      return [];
    }
    return currentValue as Object[];
  }

  private resolveTemplateItemTitle(item: Object): string | undefined {
    if (typeof item !== 'object' || item === null || Array.isArray(item)) {
      return undefined;
    }

    let record = item as Record<string, Object>;
    const candidateKeys: string[] = ['title', 'label', 'name'];
    for (const key of candidateKeys) {
      let value: Object | undefined = record[key];
      if (typeof value === 'string' && value.trim().length > 0) {
        return value;
      }
    }
    return undefined;
  }

  private hasObjectKey(target: Record<string, Object>, key: string): boolean {
    let keys: string[] = Object.keys(target);
    for (const candidate of keys) {
      if (candidate === key) {
        return true;
      }
    }
    return false;
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

  private resolveTabIndex(customProps: Record<string, Object> | undefined): number {
    if (customProps === undefined) {
      return this.currentIndex;
    }
    let resolved: number | undefined = this.resolveStrictDynamicNumberOptionValue(customProps['tabIndex']);
    return normalizeExtendedTabsIndexValue(resolved, this.currentIndex);
  }

  private dispatchTabIndexRangeWarning(tabIndex: number, itemCount: number, message: string): void {
    let warningSignature: string = `${tabIndex}:${itemCount}`;
    if (this.lastInvalidTabIndexSignature === warningSignature) {
      return;
    }
    this.lastInvalidTabIndexSignature = warningSignature;
    dispatchExtendedTabsIndexRangeWarning(this.attribute, message);
  }

  private resolveVertical(): boolean {
    let customProps = parseCustomProps(this.attribute, this.tag);
    if (customProps === undefined) {
      return false;
    }
    let resolved = resolveDynamicBooleanOptionValue(customProps['vertical'], this.attribute);
    return resolved ?? false;
  }

  private resolveScrollable(): boolean {
    let customProps = parseCustomProps(this.attribute, this.tag);
    if (customProps === undefined) {
      return true;
    }
    let resolved = resolveDynamicBooleanOptionValue(customProps['scrollable'], this.attribute);
    return resolved ?? true;
  }

  private resolveTabBarBackgroundColor(): string {
    return this.isDarkMode() ? DEFAULT_TAB_BAR_BACKGROUND_COLOR_DARK : DEFAULT_TAB_BAR_BACKGROUND_COLOR_LIGHT;
  }

  private resolveStrictDynamicNumberOptionValue(value: Object | undefined): number | undefined {
    if (value === undefined) {
      return undefined;
    }
    if (typeof value === 'number' && Number.isFinite(value)) {
      return value;
    }
    let resolveContext: A2UIResolveContext | undefined = this.buildResolveContext();
    let resolved: number = DynamicValueResolver.resolveNumber(value as A2UIDynamicNumber, Number.NaN, resolveContext);
    if (!Number.isNaN(resolved) && Number.isFinite(resolved)) {
      return resolved;
    }
    return undefined;
  }

  private buildResolveContext(): A2UIResolveContext | undefined {
    if (this.attribute === undefined) {
      return undefined;
    }
    let surfaceId: string = this.attribute.surfaceId ?? '';
    let componentId: string = this.attribute.id ?? '';
    let renderId: number = this.attribute.renderId ?? -1;
    return {
      renderId: renderId,
      surfaceId: surfaceId,
      componentId: componentId,
      dataModel: this.attribute.dataModel,
      invokeFunction: (functionCall: A2UIFunctionCall): A2UIValueType =>
        FunctionBridge.invoke(functionCall, surfaceId, componentId, renderId)
    } as A2UIResolveContext;
  }

  private resolveBarPosition(): BarPosition {
    let customProps = parseCustomProps(this.attribute, this.tag);
    if (customProps === undefined) {
      return BarPosition.Start;
    }
    let value: string | undefined = resolveDynamicStringOptionValue(customProps['barPosition'], this.attribute);
    if (value === undefined) {
      return BarPosition.Start;
    }
    let normalized = value.toLowerCase();
    if (normalized === 'end') {
      return BarPosition.End;
    }
    return BarPosition.Start;
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

  private resolveListenerOwnerKey(): string {
    let renderId = this.attribute?.renderId ?? -1;
    let surfaceId = this.resolveSurfaceId();
    let componentId = this.attribute?.id ?? 'unknown';
    return `${this.tag}:${renderId}:${surfaceId}:${componentId}`;
  }

  private resolveSurfaceId(): string {
    return this.attribute?.surfaceId ?? '';
  }

  private resolveFallbackTitle(tab: ExtendedTabsResolvedItem, index: number): string {
    if (tab.fallbackTitle !== undefined && tab.fallbackTitle.trim().length > 0) {
      return tab.fallbackTitle;
    }
    if (this.shouldUseSafeGeneratedTitle(tab.childId)) {
      return `Tab ${index + 1}`;
    }
    return tab.childId;
  }

  private shouldUseSafeGeneratedTitle(childId: string): boolean {
    if (childId.startsWith('/')) {
      return true;
    }
    return this.isTemplateInstanceChildId(childId);
  }

  private dispatchOnChange(index: number): void {
    this.dispatchOnChangeFunctionCall(index);
    let payload: Record<string, Object> = { 'index': index as Object };
    dispatchComponentEvent(this.attribute, 'onChange', payload);
  }

  private dispatchOnChangeFunctionCall(index: number): void {
    let functionCall = this.resolveOnChangeFunctionCall(index);
    if (functionCall === undefined) {
      return;
    }
    let resolveContext = this.buildResolveContext();
    if (resolveContext === undefined) {
      return;
    }
    DynamicValueResolver.resolveValue(functionCall as A2UIDynamicValue, resolveContext);
  }

  private resolveOnChangeFunctionCall(index: number): A2UIFunctionCall | undefined {
    let customProps = parseCustomProps(this.attribute, this.tag);
    if (customProps === undefined) {
      return undefined;
    }
    let rawOnChange = customProps[TABS_ON_CHANGE_PROP_NAME];
    if (!this.isRecord(rawOnChange)) {
      return undefined;
    }

    let onChangeRecord = rawOnChange as Record<string, Object>;
    let functionCallRecord: Record<string, Object> | undefined = this.resolveFunctionCallRecord(onChangeRecord);
    if (functionCallRecord === undefined) {
      return undefined;
    }

    let callName = functionCallRecord['call'] as string;
    let functionArgs = this.mergeOnChangeFunctionArgs(this.resolveFunctionCallArgs(functionCallRecord), index);
    let returnType = this.resolveFunctionCallReturnType(functionCallRecord);
    return {
      call: callName,
      args: functionArgs,
      returnType: returnType
    } as A2UIFunctionCall;
  }

  private resolveFunctionCallRecord(record: Record<string, Object>): Record<string, Object> | undefined {
    if (typeof record['call'] === 'string' && (record['call'] as string).length > 0) {
      return record;
    }
    let functionCallValue = record['functionCall'];
    if (!this.isRecord(functionCallValue)) {
      return undefined;
    }
    let functionCallRecord = functionCallValue as Record<string, Object>;
    if (typeof functionCallRecord['call'] !== 'string' || (functionCallRecord['call'] as string).length === 0) {
      return undefined;
    }
    return functionCallRecord;
  }

  private resolveFunctionCallArgs(record: Record<string, Object>): A2UIFunctionArgs | undefined {
    let argsValue = record['args'];
    if (!this.isRecord(argsValue)) {
      return undefined;
    }
    return argsValue as A2UIFunctionArgs;
  }

  private resolveFunctionCallReturnType(record: Record<string, Object>): A2UIFunctionReturnType {
    let returnTypeValue = record['returnType'];
    if (typeof returnTypeValue !== 'string') {
      return 'void';
    }
    let normalized = returnTypeValue as A2UIFunctionReturnType;
    return normalized;
  }

  private mergeOnChangeFunctionArgs(baseArgs: A2UIFunctionArgs | undefined, index: number): A2UIFunctionArgs {
    let result: A2UIFunctionArgs = {};
    if (baseArgs !== undefined) {
      let sourceArgs = baseArgs as Record<string, A2UIValueType | Record<string, Object>>;
      let keys = Object.keys(sourceArgs);
      for (const key of keys) {
        result[key] = sourceArgs[key];
      }
    }
    result['index'] = index;
    result['value'] = index;
    return result;
  }

  private isRecord(value: Object | undefined): boolean {
    return typeof value === 'object' && value !== null && !Array.isArray(value);
  }

}

@Builder
export function extendedTabsBuilder(attribute: CustomComponentAttribute): void {
  ExtendedTabs({ attribute: attribute });
}

const extendedTabsSchemaProvider: SchemaProvider = (_version: string): string => {
  return SchemaResourceLoader.loadSchema('schema/Extended/components/ExtendedTabs.json');
};

export function createExtendedTabsDefinition(): CustomComponentDefinition {
  return {
    type: EXTENDED_TABS_TYPE,
    builder: wrapBuilder(extendedTabsBuilder),
    schemaProvider: extendedTabsSchemaProvider
  };
}
