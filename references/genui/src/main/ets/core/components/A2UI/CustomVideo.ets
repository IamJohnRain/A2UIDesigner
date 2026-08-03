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
  isSupportedMediaUrl,
  parseCustomProps,
  resolveCustomComponentWarningPathPrefix,
  resolveDynamicStringOptionValue,
  withAttributeSchemaWarningContext,
  resolveMargin,
  Margin
} from './CustomComponentUtils';
import { SchemaErrorCode, SchemaErrorInfoManager } from '../../schema/SchemaErrorInfoManager';
import { SchemaResourceLoader } from '../../schema/SchemaResourceLoader';
import { CatalogItem } from '../../base/CatalogItem';
import { SchemaLocalPropertyHelper } from '../../schema/SchemaLocalPropertyHelper';
import { SchemaProvider } from '../../types/types';

export interface VideoOptions {
  url: string;
  renderContent: boolean;
}

const VIDEO_COMPONENT_TYPE: string = 'Video';
const DEFAULT_VIDEO_URL: string = '';
const VIDEO_SUPPORTED_EXTENSIONS: string[] = [
  '.mp4',
  '.m4v',
  '.mov',
  '.webm',
  '.mkv',
  '.avi',
  '.3gp',
  '.mpeg',
  '.mpg',
  '.m3u8'
];

function recordVideoWarning(code: SchemaErrorCode, message: string, path: string): void {
  SchemaErrorInfoManager.getInstance().recordSchemaWarning(code, message, path, 'component', 'Video');
}

function resolveVideoWarningPathPrefix(attribute: CustomComponentAttribute | undefined): string {
  return resolveCustomComponentWarningPathPrefix(attribute);
}

function resolveVideoWarningPath(attribute: CustomComponentAttribute | undefined, key: string): string {
  return `${resolveVideoWarningPathPrefix(attribute)}.${key}`;
}

function createDefaultVideoOptions(): VideoOptions {
  return {
    url: DEFAULT_VIDEO_URL,
    renderContent: true
  };
}

function resolveVideoUrlProperty(
  source: Record<string, Object>,
  attribute: CustomComponentAttribute | undefined,
  required: boolean = false
): string | undefined {
  const rawValue: Object | undefined = source['url'];
  const propertyPath: string = resolveVideoWarningPath(attribute, 'url');
  if (rawValue === undefined) {
    if (required) {
      recordVideoWarning(SchemaErrorCode.REQUIRED_MISS, 'Property url is required', propertyPath);
    }
    return undefined;
  }

  if (typeof rawValue === 'string') {
    if (rawValue.length > 0) {
      return rawValue;
    }
    recordVideoWarning(SchemaErrorCode.INVALID_VALUE, 'Property url expects non-empty string', propertyPath);
    return undefined;
  }

  if (typeof rawValue === 'object' && rawValue !== null && !Array.isArray(rawValue)) {
    let rawRecord: Record<string, Object> = rawValue as Record<string, Object>;
    if (typeof rawRecord['path'] === 'string' || typeof rawRecord['call'] === 'string') {
      return resolveDynamicStringOptionValue(rawValue, attribute);
    }

    if (typeof rawRecord['value'] === 'string') {
      let objectValue: string = rawRecord['value'] as string;
      if (objectValue.length > 0) {
        recordVideoWarning(
          SchemaErrorCode.TYPE_MISMATCH,
          'Property url expects DynamicString, object.value has been coerced',
          propertyPath
        );
        return objectValue;
      }
      recordVideoWarning(SchemaErrorCode.INVALID_VALUE, 'Property url expects non-empty string', propertyPath);
      return undefined;
    }
  }

  recordVideoWarning(
    SchemaErrorCode.TYPE_MISMATCH,
    'Property url expects DynamicString compatible value, fallback to default value',
    propertyPath
  );
  return undefined;
}

export function isSupportedVideoUrl(url: string): boolean {
  return isSupportedMediaUrl(url, VIDEO_SUPPORTED_EXTENSIONS, 'data:video/');
}

export function resolveVideoOptionsForSchemaWarning(
  attribute: CustomComponentAttribute | undefined
): VideoOptions {
  return withAttributeSchemaWarningContext(attribute, (): VideoOptions => {
    let parsed = parseCustomProps(attribute, 'CustomVideo');
    const warningPathPrefix: string = resolveVideoWarningPathPrefix(attribute);
    if (parsed === undefined) {
      if (attribute?.customProps === undefined || attribute?.customProps === null) {
        recordVideoWarning(SchemaErrorCode.REQUIRED_MISS, 'Property url is required', `${warningPathPrefix}.url`);
      }
      return createDefaultVideoOptions();
    }

    SchemaLocalPropertyHelper.recordUnknownFields(parsed, ['url'], 'Video', warningPathPrefix);
    let options: VideoOptions = createDefaultVideoOptions();
    let urlValue: string | undefined = resolveVideoUrlProperty(parsed, attribute, true);
    if (urlValue !== undefined && urlValue.length > 0) {
      if (!isSupportedVideoUrl(urlValue)) {
        recordVideoWarning(
          SchemaErrorCode.INVALID_VALUE,
          'Property url expects a supported video resource format, drop current component',
          `${warningPathPrefix}.url`
        );
        options.renderContent = false;
        return options;
      }
      options.url = urlValue;
    }
    return options;
  });
}

@Component
export struct CustomVideo {
  @Prop @Watch('onAttributeChange') attribute: CustomComponentAttribute | undefined = undefined;
  @State private resolvedUrl: string = DEFAULT_VIDEO_URL;
  @State private shouldRenderContent: boolean = true;
  private lastResolvedCustomProps: Record<string, Object> | undefined = undefined;
  private lastResolvedDataModel: ESObject | undefined = undefined;

  aboutToAppear(): void {
    this.synchronizeResolvedOptions();
  }

  onAttributeChange(): void {
    this.synchronizeResolvedOptions();
  }

  build() {
    if (this.shouldRenderContent) {
      Column() {
        Video({ src: this.resolvedUrl })
          .width('100%')
          .height('100%')
          .objectFit(ImageFit.Contain)
          .autoPlay(false)
          .controls(true)
      }
      .width('100%')
      .layoutWeight(this.resolveWeight())
      .margin(this.resolveMargin())
      .accessibilityGroup(true)
      .accessibilityText(this.resolveAccessibilityLabel())
      .accessibilityDescription(this.resolveAccessibilityDescription())
      .id(this.attribute?.id ?? '');
    }
  }

  private synchronizeResolvedOptions(): void {
    if (this.attribute?.customProps === this.lastResolvedCustomProps &&
        this.attribute?.dataModel === this.lastResolvedDataModel) {
      return;
    }
    let options: VideoOptions = this.resolveOptions();
    this.resolvedUrl = options.url;
    this.shouldRenderContent = options.renderContent;
    this.lastResolvedCustomProps = this.attribute?.customProps;
    this.lastResolvedDataModel = this.attribute?.dataModel;
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

  private resolveOptions(): VideoOptions {
    return resolveVideoOptionsForSchemaWarning(this.attribute);
  }
}

@Builder
export function customVideoBuilder(attribute: CustomComponentAttribute): void {
  CustomVideo({ attribute: attribute });
}

export function createVideoDefinition(): CustomComponentDefinition {
  const schemaProvider: SchemaProvider = (version: string): string => {
    return SchemaResourceLoader.loadA2UISchema(version, 'components/Video.json');
  };
  return {
    type: VIDEO_COMPONENT_TYPE,
    builder: wrapBuilder(customVideoBuilder),
    schemaProvider: schemaProvider
  };
}

export function asCatalogItem(): CatalogItem {
  const definition: CustomComponentDefinition = createVideoDefinition();
  return CatalogItem.forComponent(
    definition.type,
    definition.schemaProvider ?? ((_version: string): string => ''),
    definition.builder
  ).markInnerNative(false);
}
