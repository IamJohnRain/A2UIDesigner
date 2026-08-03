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

import { CatalogItem as PublicCatalogItem } from '../../interface/CatalogItem';
import { ComponentBuilder, CustomComponentAttribute, SchemaProvider } from '../types/types';
import { SchemaValidator } from '../schema/SchemaValidator';
import { CapabilitiesCore } from './CapabilitiesCore';

@Builder
function emptyNativeComponentBuilder(_attribute: CustomComponentAttribute): void {
}

export const EMPTY_COMPONENT_BUILDER: ComponentBuilder = wrapBuilder(emptyNativeComponentBuilder);

export enum CatalogCategory {
  // all basic components defined by A2UI protocol
  A2UI_STANDARD = 0,
  // the components implements for ohos, they extend the styles
  OHOS_EXTENDS = 1
}

/**
 * catalog item 能力类型
 */
export enum CatalogItemType {
  // 协议消息格式
  PROTOCOL_MESSAGE = 0,
  // 组件
  COMPONENT = 1,
  // 函数调用
  LOCAL_FUNCTION = 2
}

export interface CatalogItemOptions {
  category?: CatalogCategory;
  isInnerNative?: boolean;
}

/**
 * 通过这个类来定义所圈定的能力
 */
export class CatalogItem implements PublicCatalogItem {
  // Keep strict schema validation diagnostics without crashing catalog creation.
  private static readonly STRICT_SCHEMA_VALIDATION_FATAL: boolean = false;

  // 默认使用 A2UI 标准组件
  category: CatalogCategory;
  // 必选
  readonly type: CatalogItemType;
  // 必选
  readonly name: string;
  // 必选，是否为 native 侧 inner 实现，仅支持框架预置实现，默认为 ets 侧实现
  isInnerNative: boolean;
  // 必选
  readonly schemaProvider: SchemaProvider;
  // 可选，注册组件时提供，function 不提供
  readonly componentBuilder: ComponentBuilder;

  private constructor(
    name: string,
    schemaProvider: SchemaProvider,
    componentBuilder: ComponentBuilder,
    options?: CatalogItemOptions
  ) {
    this.name = name;
    this.type = CatalogItemType.COMPONENT;
    this.schemaProvider = schemaProvider;
    this.componentBuilder = componentBuilder;
    this.category = options?.category ?? CatalogCategory.OHOS_EXTENDS;
    this.isInnerNative = options?.isInnerNative ?? false;
  }

  public markCategory(category: CatalogCategory): CatalogItem {
    this.category = category;
    return this;
  }

  public markInnerNative(isNative: boolean): CatalogItem {
    this.isInnerNative = isNative;
    return this;
  }

  // 构造组件 catalog item
  public static forComponent(
    name: string,
    schemaProvider: SchemaProvider,
    componentBuilder: ComponentBuilder,
    options?: CatalogItemOptions
  ): CatalogItem {
    CatalogItem.validateStrictComponentSchema(name, schemaProvider);
    return new CatalogItem(name, schemaProvider, componentBuilder, options);
  }

  public static fromComponentItem(item: PublicCatalogItem | null | undefined): CatalogItem | undefined {
    if (item === undefined || item === null) {
      return undefined;
    }
    const name: string = item.name ?? '';
    if (name.length === 0) {
      return undefined;
    }
    if (item instanceof CatalogItem) {
      return item;
    }
    if (item.schemaProvider === undefined || item.componentBuilder === undefined) {
      return undefined;
    }
    return CatalogItem.forComponent(name, item.schemaProvider, item.componentBuilder);
  }

  private static validateStrictComponentSchema(name: string, schemaProvider: SchemaProvider): void {
    const schema: string = schemaProvider(CapabilitiesCore.getDefaultA2UIProtocolVersion()).trim();
    if (schema.length === 0) {
      return;
    }
    const errors: string[] = SchemaValidator.validateComponentSchema(name, schema);
    CatalogItem.reportComponentSchemaErrors(errors, name);
  }

  private static reportComponentSchemaErrors(
    errors: string[],
    name: string
  ): void {
    if (errors.length === 0) {
      return;
    }
    const message: string = `[CatalogItem] invalid component schema for ${name}: ${errors.join(' | ')}`;
    if (CatalogItem.STRICT_SCHEMA_VALIDATION_FATAL) {
      throw new Error(message);
    }
    console.warn(message);
  }
}
