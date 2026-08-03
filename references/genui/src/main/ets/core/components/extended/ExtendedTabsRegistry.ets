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

export type TabFontWeight = number | string;

export interface ExtendedTabMetadata {
  title: string;
  icon: string;
  selectedSrc: string;
  tabType: string;
  selectedColor?: string;
  unSelectedColor?: string;
  defaultBackgroundColor?: string;
  selectedBackgroundColor?: string;
  defaultBorderColor?: string;
  selectedBorderColor?: string;
  fontSize?: number;
  fontWeight?: TabFontWeight;
  iconSize?: number;
  space?: number;
}

export class ExtendedTabsRegistry {
  private static metadataByComponentKey: Map<string, ExtendedTabMetadata> = new Map();
  private static listenersByOwnerKey: Map<string, () => void> = new Map();
  private static readonly LEGACY_LISTENER_OWNER_KEY: string = '__legacy_listener__';

  public static updateMetadata(surfaceId: string, componentId: string, metadata: ExtendedTabMetadata): void {
    let componentKey = ExtendedTabsRegistry.buildMetadataKey(surfaceId, componentId);
    if (componentKey.length === 0) {
      return;
    }
    let previous = ExtendedTabsRegistry.metadataByComponentKey.get(componentKey);
    if (previous !== undefined && ExtendedTabsRegistry.isSameMetadata(previous, metadata)) {
      return;
    }
    ExtendedTabsRegistry.metadataByComponentKey.set(componentKey, metadata);
    ExtendedTabsRegistry.notifyListeners();
  }

  public static removeMetadata(surfaceId: string, componentId: string): void {
    let componentKey = ExtendedTabsRegistry.buildMetadataKey(surfaceId, componentId);
    if (componentKey.length === 0) {
      return;
    }
    if (ExtendedTabsRegistry.metadataByComponentKey.delete(componentKey)) {
      ExtendedTabsRegistry.notifyListeners();
    }
  }

  public static getMetadata(surfaceId: string, componentId: string): ExtendedTabMetadata | undefined {
    let componentKey = ExtendedTabsRegistry.buildMetadataKey(surfaceId, componentId);
    if (componentKey.length === 0) {
      return undefined;
    }
    return ExtendedTabsRegistry.metadataByComponentKey.get(componentKey);
  }

  public static addListener(listener: () => void, ownerKey?: string): void {
    let resolvedOwnerKey = ownerKey;
    if (resolvedOwnerKey === undefined || resolvedOwnerKey.length === 0) {
      resolvedOwnerKey = ExtendedTabsRegistry.LEGACY_LISTENER_OWNER_KEY;
    }
    ExtendedTabsRegistry.listenersByOwnerKey.set(resolvedOwnerKey, listener);
  }

  public static removeListener(listener: () => void, ownerKey?: string): void {
    let resolvedOwnerKey = ownerKey;
    if (resolvedOwnerKey !== undefined && resolvedOwnerKey.length > 0) {
      let currentListener = ExtendedTabsRegistry.listenersByOwnerKey.get(resolvedOwnerKey);
      if (currentListener === listener) {
        ExtendedTabsRegistry.listenersByOwnerKey.delete(resolvedOwnerKey);
      }
      return;
    }

    ExtendedTabsRegistry.listenersByOwnerKey.forEach((registeredListener: () => void, key: string) => {
      if (registeredListener === listener) {
        ExtendedTabsRegistry.listenersByOwnerKey.delete(key);
      }
    });
  }

  private static notifyListeners(): void {
    ExtendedTabsRegistry.listenersByOwnerKey.forEach((listener: () => void) => {
      listener();
    });
  }

  private static buildMetadataKey(surfaceId: string, componentId: string): string {
    if (componentId.length === 0) {
      return '';
    }
    if (surfaceId.length === 0) {
      return componentId;
    }
    return `${surfaceId}::${componentId}`;
  }

  private static isSameMetadata(left: ExtendedTabMetadata, right: ExtendedTabMetadata): boolean {
    return left.title === right.title &&
      left.icon === right.icon &&
      left.selectedSrc === right.selectedSrc &&
      left.tabType === right.tabType &&
      left.selectedColor === right.selectedColor &&
      left.unSelectedColor === right.unSelectedColor &&
      left.defaultBackgroundColor === right.defaultBackgroundColor &&
      left.selectedBackgroundColor === right.selectedBackgroundColor &&
      left.defaultBorderColor === right.defaultBorderColor &&
      left.selectedBorderColor === right.selectedBorderColor &&
      left.fontSize === right.fontSize &&
      left.fontWeight === right.fontWeight &&
      left.iconSize === right.iconSize &&
      left.space === right.space;
  }
}
