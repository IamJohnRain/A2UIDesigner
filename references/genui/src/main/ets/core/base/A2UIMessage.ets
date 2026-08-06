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

import { CapabilitiesCore } from './CapabilitiesCore';

export enum A2UIMessageType {
  CREATE_SURFACE = 0,
  UPDATE_COMPONENTS = 1,
  UPDATE_DATA_MODEL = 2,
  DELETE_SURFACE = 3
}

type A2UIJsonField = ESObject | string | number | boolean | Object | null | undefined;

export class A2UIMessage {
  public version: string;
  public surfaceId: string;
  public type: A2UIMessageType;
  public messageBody?: string;

  constructor(versionNum: string, surfaceId: string, type: A2UIMessageType, body: ESObject | string | undefined) {
    this.version = versionNum;
    this.surfaceId = surfaceId;
    this.type = type;
    this.messageBody = A2UIMessage.serializeMessageBody(body);
  }

  static fromDSL(dsl: string): A2UIMessage | null {
    if (dsl.trim().length === 0) {
      console.error('[A2UIMessage] dsl is empty');
      return null;
    }

    let jsonObj: ESObject | null = null;
    try {
      jsonObj = JSON.parse(dsl);
    } catch (e) {
      console.error('[A2UIMessage] fromDSL JSON.parse FAIL' + e);
      return null;
    }

    if (!A2UIMessage.isValidJsonObject(jsonObj)) {
      console.error('[A2UIMessage] root json must be an object');
      return null;
    }

    let rootObj: ESObject = jsonObj as ESObject;
    let versionValue: A2UIJsonField = rootObj.version;
    if (typeof versionValue !== 'string' || !CapabilitiesCore.isSupportedA2UIProtocolVersion(versionValue)) {
      console.error('[A2UIMessage] version is invalid');
      return null;
    }

    let messageCount: number = 0;
    let messageKey: string = '';
    let messageType: A2UIMessageType = A2UIMessageType.CREATE_SURFACE;
    if (rootObj.createSurface !== undefined) {
      messageCount++;
      messageKey = 'createSurface';
      messageType = A2UIMessageType.CREATE_SURFACE;
    }
    if (rootObj.updateComponents !== undefined) {
      messageCount++;
      messageKey = 'updateComponents';
      messageType = A2UIMessageType.UPDATE_COMPONENTS;
    }
    if (rootObj.updateDataModel !== undefined) {
      messageCount++;
      messageKey = 'updateDataModel';
      messageType = A2UIMessageType.UPDATE_DATA_MODEL;
    }
    if (rootObj.deleteSurface !== undefined) {
      messageCount++;
      messageKey = 'deleteSurface';
      messageType = A2UIMessageType.DELETE_SURFACE;
    }

    if (messageCount !== 1) {
      console.error('[A2UIMessage] message type is invalid');
      return null;
    }

    return A2UIMessage.tryParseMessage(rootObj, versionValue, messageKey, messageType);
  }

  private static tryParseMessage(
    jsonObj: ESObject,
    version: string,
    key: string,
    type: A2UIMessageType
  ): A2UIMessage | null {
    let bodyValue: A2UIJsonField = jsonObj[key];
    if (!A2UIMessage.isValidJsonObject(bodyValue)) {
      console.error(`[A2UIMessage] ${key} body is invalid`);
      return null;
    }

    let body: ESObject = bodyValue as ESObject;
    let surfaceIdValue: A2UIJsonField = body.surfaceId;
    if (typeof surfaceIdValue !== 'string' || surfaceIdValue.trim().length === 0) {
      console.error(`[A2UIMessage] ${key}.surfaceId is invalid`);
      return null;
    }

    if (!A2UIMessage.validateMessageBody(body, key, type)) {
      return null;
    }

    return new A2UIMessage(version, surfaceIdValue as string, type, body);
  }

  private static validateMessageBody(body: ESObject, key: string, type: A2UIMessageType): boolean {
    switch (type) {
      case A2UIMessageType.UPDATE_COMPONENTS:
        if (!Array.isArray(body.components)) {
          console.error(`[A2UIMessage] ${key}.components is invalid`);
          return false;
        }
        return true;
      case A2UIMessageType.UPDATE_DATA_MODEL:
        if (body.path === undefined && body.value === undefined) {
          console.error(`[A2UIMessage] ${key} requires path or value`);
          return false;
        }
        return true;
      default:
        return true;
    }
  }

  private static isValidJsonObject(value: A2UIJsonField): boolean {
    return value !== null && value !== undefined && typeof value === 'object' && !Array.isArray(value);
  }

  private static serializeMessageBody(body: ESObject | string | undefined): string | undefined {
    if (body === undefined) {
      return undefined;
    }
    if (typeof body === 'string') {
      return body;
    }
    return JSON.stringify(body);
  }

}
