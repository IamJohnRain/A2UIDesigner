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
import { parseCustomProps, resolveDynamicStringOptionValue, dispatchComponentEvent } from '../A2UI/CustomComponentUtils';
import { SchemaResourceLoader } from '../../schema/SchemaResourceLoader';
import { SchemaProvider } from '../../types/types';
import { webview } from '@kit.ArkWeb';
import { buildExtendedCommonStyleModifier } from './ExtendedCommonStyleModifier';

interface ExtendedWebOptions {
  url: string;
}

const EXTENDED_WEB_COMPONENT_TYPE: string = 'Web';
const DEFAULT_WEB_URL: string = '';

@Component
export struct ExtendedWeb {
  @Prop @Watch('onAttributeChange') attribute: CustomComponentAttribute | undefined = undefined;
  @State currentUrl: string = '';
  webviewController: webview.WebviewController = new webview.WebviewController();

  aboutToAppear(): void {
    console.log(`ExtendedWeb: aboutToAppear, attribute=${JSON.stringify(this.attribute)}`);
    this.updateUrl();
  }

  onAttributeChange(): void {
    console.log(`ExtendedWeb: onAttributeChange, attribute=${JSON.stringify(this.attribute)}`);
    this.updateUrl();
  }

  private updateUrl(): void {
    let url = this.resolveUrl();
    console.log(`ExtendedWeb: updateUrl, resolved url=${url}`);
    let previousUrl: string = this.currentUrl;
    this.currentUrl = url;

    if (url.length === 0 || previousUrl.length === 0 || previousUrl === url) {
      return;
    }

    this.loadUrlSafely(url);
  }

build() {
  Web({ src: this.currentUrl, controller: this.webviewController })
    .layoutWeight(this.resolveWeight())
    .onClick(() => {
      dispatchComponentEvent(this.attribute, 'onClick');
    })
    .onAppear(() => {
      dispatchComponentEvent(this.attribute, 'onAppear');
    })
    .accessibilityGroup(true)
    .accessibilityText(this.resolveAccessibilityLabel())
    .accessibilityDescription(this.resolveAccessibilityDescription())
    .id(this.attribute?.id ?? '')
    .attributeModifier(buildExtendedCommonStyleModifier<WebAttribute>(this.attribute));
}

  private resolveWeight(): number {
    let weight = this.attribute?.properties?.weight;
    if (weight !== undefined && weight > 0) {
      return weight;
    }
    return 0;
  }

  private resolveUrl(): string {
    return this.resolveOptions().url;
  }

  private resolveAccessibilityLabel(): string {
    return this.attribute?.properties?.accessibilityLabel ?? '';
  }

  private resolveAccessibilityDescription(): string {
    return this.attribute?.properties?.accessibilityDescription ?? '';
  }

  private resolveOptions(): ExtendedWebOptions {
    let parsed = parseCustomProps(this.attribute, 'ExtendedWeb');
    console.log(`ExtendedWeb: resolveOptions, parsed=${JSON.stringify(parsed)}, dataModel=${JSON.stringify(this.attribute?.dataModel)}`);
    if (parsed === undefined) {
      return { url: DEFAULT_WEB_URL };
    }

    let urlValue: string | undefined = resolveDynamicStringOptionValue(parsed['url'], this.attribute);
    console.log(`ExtendedWeb: resolveOptions, urlValue=${urlValue}`);
    return {
      url: urlValue !== undefined ? urlValue : DEFAULT_WEB_URL
    };
  }

  private loadUrlSafely(url: string): void {
    try {
      this.webviewController.loadUrl(url);
      console.log(`ExtendedWeb: loadUrl success, url=${url}`);
    } catch (error) {
      console.warn(`ExtendedWeb: loadUrl failed, url=${url}, error=${JSON.stringify(error)}`);
    }
  }
}

@Builder
export function extendedWebBuilder(attribute: CustomComponentAttribute): void {
  ExtendedWeb({ attribute: attribute });
}

export function createExtendedWebDefinition(): CustomComponentDefinition {
  const schemaProvider: SchemaProvider = (_version: string): string => {
    return SchemaResourceLoader.loadSchema('schema/Extended/components/ExtendedWeb.json');
  };

  return {
    type: EXTENDED_WEB_COMPONENT_TYPE,
    builder: wrapBuilder(extendedWebBuilder),
    schemaProvider: schemaProvider
  };
}
