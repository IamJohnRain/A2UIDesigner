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

import nativeEngine from 'liba2ui_native.so';
import { ComponentContent, NodeContent, UIContext } from '@kit.ArkUI';
import { CatalogItem } from '../../base/CatalogItem';
import { A2UIValueType, SchemaProvider, ChangeReason } from '../../types/types';
import { FunctionBridge } from '../../functions/FunctionBridge';
import { A2UIFunctionCall } from '../../types/FunctionCall';
import { CustomComponentDynamicValueResolver } from '../../types/runtime/CustomComponentDynamicValueResolver';
import {
  CommonStyleProps,
  CustomComponentAttribute,
  CustomComponentDescriptor,
  CustomComponentResult,
  ComponentTheme
} from './CustomComponent';

const emptySchemaProvider: SchemaProvider = (_version: string): string => {
  return '';
};

export interface CustomComponentDefinition {
  type: string;
  builder: WrappedBuilder<[CustomComponentAttribute]>;
  schemaProvider?: SchemaProvider;
}

interface MultiSlotsEnsureResult {
  slots: Map<string, NodeContent>;
  slotsObject: Record<string, NodeContent>;
  createdNewSlots: boolean;
}

export interface CustomComponentAttributeBuildOptions {
  builder: WrappedBuilder<[CustomComponentAttribute]>;
  changeReason?: ChangeReason;
  dataModel?: ESObject;
  slot?: NodeContent;
  slots?: Map<string, NodeContent>;
}

interface CustomComponentAttributeSnapshot {
  descriptorJson: string;
  componentThemeJson: string;
  colorMode?: number;
  breakpoint?: number;
}

interface CustomComponentDescriptorSnapshotPayload {
  customProps: Record<string, Object> | null;
  dataModelJson: string;
  properties: CommonStyleProps | null;
}

export function buildCustomComponentAttribute(
  value: CustomComponentDescriptor,
  options: CustomComponentAttributeBuildOptions
): CustomComponentAttribute {
  const surfaceId: string = value.surfaceId ?? '';
  const componentId: string = value.id ?? '';
  const customComponentHandle: number = value.customComponentHandle ?? -1;
  return {
    type: value.type,
    id: componentId,
    surfaceId: surfaceId,
    protocolVersion: value.protocolVersion ?? '',
    catalogId: value.catalogId ?? '',
    renderId: value.renderId,
    properties: value.properties,
    componentTheme: value.componentTheme ?? {},
    customProps: value.customProps,
    changeReason: options.changeReason ?? ChangeReason.UPDATE_COMPONENT,
    resolver: new CustomComponentDynamicValueResolver({
      customComponentHandle: customComponentHandle,
      surfaceId: surfaceId,
      componentId: componentId,
      renderId: value.renderId,
      catalogId: value.catalogId ?? '',
      protocolVersion: value.protocolVersion ?? '',
      dataModel: options.dataModel,
      invokeFunction: (functionCall: A2UIFunctionCall): A2UIValueType => {
        return FunctionBridge.invoke(functionCall, surfaceId, componentId, value.renderId);
      }
    }),
    dataModelJson: value.dataModelJson,
    dataModel: options.dataModel,
    slot: options.slot,
    slots: options.slots,
    builder: options.builder
  };
}

export class CustomComponentFactory {
  private static definitions: Map<string, CustomComponentDefinition> = new Map();
  private static bridgeRegistered: boolean = false;
  private static currentUIContext: UIContext | undefined = undefined;
  private static attributeSnapshots: WeakMap<
    ComponentContent<CustomComponentAttribute>,
    CustomComponentAttributeSnapshot
  > = new WeakMap<ComponentContent<CustomComponentAttribute>, CustomComponentAttributeSnapshot>();

  public static InstallCustomComponent(uiContext: UIContext, definitions?: Array<CustomComponentDefinition>): void {
    CustomComponentFactory.currentUIContext = uiContext;
    CustomComponentFactory.ensureBridgeRegistered();

    if (definitions === undefined) {
      return;
    }

    for (let def of definitions) {
      if (!def || !def.type || !def.builder) {
        console.log('InstallCustomComponent Invalid Definition');
        continue;
      }
      CustomComponentFactory.registerCustomComponent(def);
    }
  }

  public static registerCustomComponent(definition: CustomComponentDefinition): void {
    if (definition.type.length === 0) {
      return;
    }
    CustomComponentFactory.ensureBridgeRegistered();
    CustomComponentFactory.definitions.set(definition.type, definition);
  }

  public static registerCatalogComponents(catalogItems: CatalogItem[]): void {
    for (let item of catalogItems) {
      if (item === undefined || item === null || item.isInnerNative) {
        continue;
      }
      let builder = item.componentBuilder as WrappedBuilder<[CustomComponentAttribute]> | undefined;
      if (builder === undefined) {
        continue;
      }
      CustomComponentFactory.registerCustomComponent({
        type: item.name,
        builder: builder,
        schemaProvider: item.schemaProvider
      });
    }
  }

  private static ensureBridgeRegistered(): void {
    if (CustomComponentFactory.bridgeRegistered) {
      return;
    }
    let engineObject = nativeEngine as Object | undefined;
    if (engineObject !== undefined && engineObject !== null) {
      nativeEngine.registerCreateCustomComponent(CustomComponentFactory.createCustomComponent);
      nativeEngine.registerUpdateCustomComponent(CustomComponentFactory.updateCustomComponent);
      CustomComponentFactory.bridgeRegistered = true;
    }
  }

  public static getCurrentUIContext(): UIContext | undefined {
    return CustomComponentFactory.currentUIContext;
  }

  public static getBuilder(type: string): WrappedBuilder<[CustomComponentAttribute]> | undefined {
    return CustomComponentFactory.findDefinition(type)?.builder;
  }

  private static findDefinition(type: string): CustomComponentDefinition | undefined {
    let definition = CustomComponentFactory.definitions.get(type);
    if (definition !== undefined) {
      return definition;
    }
    let shortType = CustomComponentFactory.getShortType(type);
    if (shortType !== type) {
      return CustomComponentFactory.definitions.get(shortType);
    }
    return undefined;
  }

  private static getShortType(type: string): string {
    let parts = type.split('.');
    if (parts.length === 0) {
      return type;
    }
    return parts[parts.length - 1];
  }

  private static isTabsType(type: string): boolean {
    return CustomComponentFactory.getShortType(type) === 'Tabs';
  }

  private static isRowType(type: string): boolean {
    return CustomComponentFactory.getShortType(type) === 'Row';
  }

  private static supportsMultiSlots(type: string): boolean {
    return CustomComponentFactory.isTabsType(type) || CustomComponentFactory.isRowType(type);
  }

  private static getMultiSlotPrefix(type: string): string {
    if (CustomComponentFactory.isRowType(type)) {
      return 'row-';
    }
    return 'tab-';
  }

  private static parseDataModel(dataModelJson: string | undefined): ESObject | undefined {
    if (dataModelJson === undefined || dataModelJson.length === 0) {
      return undefined;
    }
    try {
      return JSON.parse(dataModelJson) as ESObject;
    } catch (error) {
      console.error(`parseDataModel failed: ${JSON.stringify(error)}`);
      return undefined;
    }
  }

  private static stringifySnapshotValue(value: Object | string | undefined): string {
    try {
      return JSON.stringify(value ?? null) ?? 'null';
    } catch (error) {
      return 'null';
    }
  }

  private static createAttributeSnapshot(value: CustomComponentDescriptor): CustomComponentAttributeSnapshot {
    const componentTheme: ComponentTheme = value.componentTheme ?? {};
    const descriptor: CustomComponentDescriptorSnapshotPayload = {
      customProps: value.customProps ?? null,
      dataModelJson: value.dataModelJson ?? '',
      properties: value.properties ?? null
    };
    return {
      descriptorJson: CustomComponentFactory.stringifySnapshotValue(descriptor),
      componentThemeJson: CustomComponentFactory.stringifySnapshotValue(componentTheme),
      colorMode: componentTheme.colorMode,
      breakpoint: componentTheme.breakpoint
    };
  }

  private static resolveChangeReason(
    previousSnapshot: CustomComponentAttributeSnapshot | undefined,
    nextSnapshot: CustomComponentAttributeSnapshot
  ): ChangeReason {
    if (previousSnapshot === undefined) {
      return ChangeReason.UPDATE_COMPONENT;
    }
    if (previousSnapshot.descriptorJson !== nextSnapshot.descriptorJson) {
      return ChangeReason.UPDATE_COMPONENT;
    }
    if (previousSnapshot.colorMode !== nextSnapshot.colorMode) {
      return ChangeReason.THEME_MODE_CHANGE;
    }
    if (previousSnapshot.breakpoint !== nextSnapshot.breakpoint) {
      return ChangeReason.BREAKPOINT_CHANGE;
    }
    if (previousSnapshot.componentThemeJson !== nextSnapshot.componentThemeJson) {
      return ChangeReason.THEME_MODE_CHANGE;
    }
    return ChangeReason.UPDATE_COMPONENT;
  }

  private static rememberAttributeSnapshot(
    content: ComponentContent<CustomComponentAttribute>,
    value: CustomComponentDescriptor
  ): void {
    CustomComponentFactory.attributeSnapshots.set(content, CustomComponentFactory.createAttributeSnapshot(value));
  }

  private static resolveUpdateChangeReason(
    content: ComponentContent<CustomComponentAttribute>,
    value: CustomComponentDescriptor
  ): ChangeReason {
    const nextSnapshot: CustomComponentAttributeSnapshot = CustomComponentFactory.createAttributeSnapshot(value);
    const previousSnapshot: CustomComponentAttributeSnapshot | undefined =
      CustomComponentFactory.attributeSnapshots.get(content);
    return CustomComponentFactory.resolveChangeReason(previousSnapshot, nextSnapshot);
  }

  private static getTabsCount(customProps: Record<string, Object> | undefined): number {
    if (customProps === undefined || customProps === null) {
      return 0;
    }
    let tabsArray = customProps['tabs'];
    return Array.isArray(tabsArray) ? tabsArray.length : 0;
  }

  private static resolveChildrenTemplateCount(
    childrenValue: Object | undefined,
    dataModel: ESObject | undefined
  ): number {
    if (typeof childrenValue !== 'object' || childrenValue === null || Array.isArray(childrenValue)) {
      return 0;
    }
    let templateValue = childrenValue as Record<string, Object>;
    let pathValue: Object | undefined = templateValue['path'];
    if (typeof pathValue !== 'string' || pathValue.length === 0) {
      return 0;
    }
    let resolvedValue: Object | undefined = CustomComponentFactory.readPath(dataModel, pathValue);
    return Array.isArray(resolvedValue) ? resolvedValue.length : 0;
  }

  private static readPath(dataModel: ESObject | undefined, path: string): Object | undefined {
    if (dataModel === undefined) {
      return undefined;
    }
    if (path.length === 0 || path === '/') {
      return dataModel as Object;
    }

    let normalizedPath: string = path.startsWith('/') ? path.substring(1) : path;
    if (normalizedPath.length === 0) {
      return dataModel as Object;
    }

    let current: Object | undefined = dataModel as Object;
    for (const rawToken of normalizedPath.split('/')) {
      let token: string = rawToken.replace(/~1/g, '/').replace(/~0/g, '~');
      if (Array.isArray(current)) {
        let index: number = Number(token);
        if (!Number.isInteger(index) || index < 0 || index >= current.length) {
          return undefined;
        }
        current = current[index] as Object;
        continue;
      }
      if (typeof current !== 'object' || current === null) {
        return undefined;
      }
      let record = current as Record<string, Object>;
      current = record[token];
      if (current === undefined) {
        return undefined;
      }
    }
    return current;
  }

  private static getExtendedTabsCount(
    customProps: Record<string, Object> | undefined,
    dataModel: ESObject | undefined
  ): number {
    if (customProps === undefined || customProps === null) {
      return 0;
    }
    let childrenValue = customProps['children'];
    if (Array.isArray(childrenValue)) {
      return childrenValue.length;
    }
    return CustomComponentFactory.resolveChildrenTemplateCount(childrenValue, dataModel);
  }

  private static getChildrenCount(
    customProps: Record<string, Object> | undefined,
    dataModel: ESObject | undefined
  ): number {
    if (customProps === undefined || customProps === null) {
      return 0;
    }
    let childrenArray = customProps['children'];
    if (Array.isArray(childrenArray)) {
      return childrenArray.length;
    }
    return CustomComponentFactory.resolveChildrenTemplateCount(childrenArray, dataModel);
  }

  private static getRequiredSlotCount(
    type: string,
    customProps: Record<string, Object> | undefined,
    dataModel: ESObject | undefined
  ): number {
    if (CustomComponentFactory.isTabsType(type)) {
      let childrenCount = CustomComponentFactory.getExtendedTabsCount(customProps, dataModel);
      if (childrenCount > 0) {
        return childrenCount;
      }
      return CustomComponentFactory.getTabsCount(customProps);
    }
    if (CustomComponentFactory.isRowType(type)) {
      return CustomComponentFactory.getChildrenCount(customProps, dataModel);
    }
    return 0;
  }

  private static hasExplicitChildren(customProps: Record<string, Object> | undefined): boolean {
    if (customProps === undefined || customProps === null) {
      return false;
    }
    for (const key of Object.keys(customProps)) {
      if (key === 'children') {
        return true;
      }
    }
    return false;
  }

  private static shouldCreateFallbackSlot(type: string, customProps: Record<string, Object> | undefined): boolean {
    if (CustomComponentFactory.isTabsType(type) && CustomComponentFactory.hasExplicitChildren(customProps)) {
      return false;
    }
    return true;
  }

  private static buildSlotsObject(slots: Map<string, NodeContent>): Record<string, NodeContent> {
    let slotsObject: Record<string, NodeContent> = {};
    slots.forEach((slot: NodeContent, key: string) => {
      slotsObject[key] = slot;
    });
    return slotsObject;
  }

  private static normalizeMultiSlots(
    childSlots: Map<string, NodeContent> | Record<string, NodeContent> | undefined
  ): Map<string, NodeContent> {
    if (childSlots instanceof Map) {
      return new Map(childSlots);
    }

    let normalizedSlots: Map<string, NodeContent> = new Map();
    if (childSlots !== undefined) {
      Object.keys(childSlots).forEach((key: string) => {
        normalizedSlots.set(key, childSlots[key]);
      });
    }
    return normalizedSlots;
  }

  private static ensureMultiSlots(
    childSlots: Map<string, NodeContent> | Record<string, NodeContent> | undefined,
    type: string,
    customProps: Record<string, Object> | undefined,
    dataModel: ESObject | undefined
  ): MultiSlotsEnsureResult {
    let slots: Map<string, NodeContent> = CustomComponentFactory.normalizeMultiSlots(childSlots);
    let requiredCount: number = CustomComponentFactory.getRequiredSlotCount(type, customProps, dataModel);
    let slotPrefix: string = CustomComponentFactory.getMultiSlotPrefix(type);
    let createdNewSlots: boolean = false;

    if (requiredCount <= 0 && slots.size === 0 && CustomComponentFactory.shouldCreateFallbackSlot(type, customProps)) {
      requiredCount = 1;
    }

    for (let i = 0; i < requiredCount; i++) {
      let slotKey = `${slotPrefix}${i}`;
      if (!slots.has(slotKey)) {
        slots.set(slotKey, new NodeContent());
        createdNewSlots = true;
      }
    }

    let result: MultiSlotsEnsureResult = {
      slots: slots,
      slotsObject: CustomComponentFactory.buildSlotsObject(slots),
      createdNewSlots: createdNewSlots
    };
    return result;
  }

  public static createCustomComponent(value: CustomComponentDescriptor): CustomComponentResult {
    console.log("ArkTS createCustomComponent");
    if (CustomComponentFactory.currentUIContext === undefined) {
      return {};
    }

    let builder = CustomComponentFactory.getBuilder(value.type);
    if (builder === undefined) {
      return {};
    }

    let dataModel: ESObject | undefined = CustomComponentFactory.parseDataModel(value.dataModelJson);

    // Special handling for components with multiple slots
    if (CustomComponentFactory.supportsMultiSlots(value.type)) {
      return CustomComponentFactory.createMultiSlotComponent(value, builder);
    }

    // Standard component creation with single slot
    let childSlot: NodeContent = new NodeContent();
    let attribute: CustomComponentAttribute = buildCustomComponentAttribute(value, {
      builder: builder,
      dataModel: dataModel,
      slot: childSlot
    });
    let content = new ComponentContent<CustomComponentAttribute>(
      CustomComponentFactory.currentUIContext,
      builder,
      attribute
    );
    CustomComponentFactory.rememberAttributeSnapshot(content, value);
    return { content: content, childSlot: childSlot };
  }

  private static createMultiSlotComponent(
    value: CustomComponentDescriptor,
    builder: WrappedBuilder<[CustomComponentAttribute]>
  ): CustomComponentResult {
    if (CustomComponentFactory.currentUIContext === undefined) {
      return {};
    }

    let dataModel: ESObject | undefined = CustomComponentFactory.parseDataModel(value.dataModelJson);
    let ensuredSlots = CustomComponentFactory.ensureMultiSlots(
      undefined,
      value.type,
      value.customProps,
      dataModel
    );
    let multiSlots: Map<string, NodeContent> = ensuredSlots.slots;

    let attribute: CustomComponentAttribute = buildCustomComponentAttribute(value, {
      builder: builder,
      dataModel: dataModel,
      slots: multiSlots
    });

    let content = new ComponentContent<CustomComponentAttribute>(
      CustomComponentFactory.currentUIContext,
      builder,
      attribute
    );
    CustomComponentFactory.rememberAttributeSnapshot(content, value);

    let result: CustomComponentResult = {
      content: content,
      childSlots: multiSlots,
      childSlotsObject: ensuredSlots.slotsObject
    };

    return result;
  }

  public static updateCustomComponent(
    customComponent: ComponentContent<CustomComponentAttribute>,
    childSlot: NodeContent,
    childSlots: Map<string, NodeContent> | Record<string, NodeContent> | undefined,
    value: CustomComponentDescriptor
  ): CustomComponentResult {
    if (value === undefined || value === null) {
      console.warn('updateCustomComponent: value is undefined or null');
      return {};
    }
    if (value.type === undefined) {
      console.warn('updateCustomComponent: value.type is undefined');
      return {};
    }
    let builder = CustomComponentFactory.getBuilder(value.type);
    if (builder === undefined) {
      return {};
    }

    let dataModel: ESObject | undefined = CustomComponentFactory.parseDataModel(value.dataModelJson);

    // Special handling for components with multiple slots
    if (CustomComponentFactory.supportsMultiSlots(value.type)) {
      let ensuredSlots = CustomComponentFactory.ensureMultiSlots(
        childSlots,
        value.type,
        value.customProps,
        dataModel
      );
      const changeReason: ChangeReason = CustomComponentFactory.resolveUpdateChangeReason(customComponent, value);
      customComponent.update(buildCustomComponentAttribute(value, {
        builder: builder,
        changeReason: changeReason,
        dataModel: dataModel,
        slots: ensuredSlots.slots
      }));
      CustomComponentFactory.rememberAttributeSnapshot(customComponent, value);
      return ensuredSlots.createdNewSlots
        ? { childSlots: ensuredSlots.slots, childSlotsObject: ensuredSlots.slotsObject }
        : {};
    }

    // Standard component update with single slot
    const changeReason: ChangeReason = CustomComponentFactory.resolveUpdateChangeReason(customComponent, value);
    customComponent.update(buildCustomComponentAttribute(value, {
      builder: builder,
      changeReason: changeReason,
      dataModel: dataModel,
      slot: childSlot
    }));
    CustomComponentFactory.rememberAttributeSnapshot(customComponent, value);
    return {};
  }

  public static createCatalogItem(type: string): CatalogItem {
    let definition = CustomComponentFactory.findDefinition(type);
    if (definition === undefined) {
      throw new Error(`Custom component is not registered: ${type}`);
    }

    let catalogItem = CatalogItem.forComponent(
      definition.type,
      definition.schemaProvider ?? emptySchemaProvider,
      definition.builder
    );
    return catalogItem.markInnerNative(false);
  }
}
