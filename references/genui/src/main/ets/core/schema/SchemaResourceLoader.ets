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

import { common } from '@kit.AbilityKit';
import { util } from '@kit.ArkTS';
import { CapabilitiesCore } from '../base/CapabilitiesCore';

export class SchemaResourceLoader {
  private static hostContext: common.Context | undefined = undefined;
  private static templateProvider: ((relativePath: string) => string | undefined) | undefined = undefined;
  private static templateCache: Map<string, string> = new Map();
  private static errorReporter: ((message: string) => void) | undefined = undefined;

  private constructor() {
  }

  public static setHostContext(hostContext: common.Context | undefined): void {
    SchemaResourceLoader.hostContext = hostContext;
  }

  public static setTemplateProvider(
    templateProvider: ((relativePath: string) => string | undefined) | undefined
  ): void {
    SchemaResourceLoader.templateProvider = templateProvider;
  }

  public static setErrorReporter(errorReporter: ((message: string) => void) | undefined): void {
    SchemaResourceLoader.errorReporter = errorReporter;
  }

  public static loadSchema(relativePath: string): string {
    return SchemaResourceLoader.loadTemplate(relativePath);
  }

  public static loadA2UISchema(version: string | undefined, relativePath: string): string {
    const resolvedVersion: string | null = CapabilitiesCore.resolveA2UIProtocolVersion(version);
    if (resolvedVersion === null) {
      const requestedVersion: string = version === undefined ? '' : version.trim();
      SchemaResourceLoader.reportError(`[SchemaResourceLoader] unsupported A2UI version: ${requestedVersion}`);
      return '';
    }

    const normalizedRelativePath: string = relativePath.trim().replace(/\\/g, '/');
    if (normalizedRelativePath.length === 0 || normalizedRelativePath.startsWith('/') ||
      normalizedRelativePath.indexOf('..') >= 0) {
      SchemaResourceLoader.reportError(
        `[SchemaResourceLoader] failed to resolve A2UI schema path, version=${resolvedVersion}, path=${relativePath}`
      );
      return '';
    }

    const resolvedPath: string = `schema/A2UI/${resolvedVersion}/${normalizedRelativePath}`;
    const schema: string = SchemaResourceLoader.loadTemplate(resolvedPath);
    if (schema.length === 0) {
      return '';
    }
    return SchemaResourceLoader.attachProtocolVersion(schema, resolvedVersion);
  }

  private static loadTemplate(relativePath: string): string {
    if (SchemaResourceLoader.templateProvider !== undefined) {
      const template: string | undefined = SchemaResourceLoader.templateProvider(relativePath);
      if (template !== undefined) {
        return template.trim();
      }
    }
    if (SchemaResourceLoader.hostContext === undefined) {
      SchemaResourceLoader.reportError(`[SchemaResourceLoader] host context is not ready, path=${relativePath}`);
      return '';
    }

    try {
      const content: Uint8Array = SchemaResourceLoader.hostContext.resourceManager.getRawFileContentSync(relativePath);
      const textDecoderOptions: util.TextDecoderOptions = {
        fatal: false,
        ignoreBOM: true
      };
      const decodeToStringOptions: util.DecodeToStringOptions = {
        stream: false
      };
      const textDecoder = util.TextDecoder.create('utf-8', textDecoderOptions);
      const template: string = textDecoder.decodeToString(content, decodeToStringOptions).trim();
      return template;
    } catch (error) {
      SchemaResourceLoader.reportError(
        `[SchemaResourceLoader] load schema failed, path=${relativePath}, error=${JSON.stringify(error)}`
      );
      return '';
    }
  }

  private static attachProtocolVersion(schema: string, version: string): string {
    try {
      const parsed: Object = JSON.parse(schema) as Object;
      if (parsed === null || Array.isArray(parsed) || typeof parsed !== 'object') {
        return schema;
      }

      const record: Record<string, Object> = parsed as Record<string, Object>;
      record['protocolVersion'] = version;
      return JSON.stringify(record, null, 2);
    } catch (_error) {
      return schema;
    }
  }

  private static reportError(message: string): void {
    console.error(message);
    if (SchemaResourceLoader.errorReporter !== undefined) {
      SchemaResourceLoader.errorReporter(message);
    }
  }
}
