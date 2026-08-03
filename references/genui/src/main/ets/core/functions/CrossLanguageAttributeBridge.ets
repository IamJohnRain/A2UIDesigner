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

import { FrameNode, UIContext, typeNode } from '@kit.ArkUI';

import { ArkUILayoutPolicyType, LayoutPolicyPayload } from '../types/ArkUILayoutPolicyType';

interface PendingCrossLanguageAttributeRequest {
  renderId: number;
  componentId: string;
  nodeUniqueId: number;
  componentType: string;
  attributeName: string;
  floatValue: number;
  stringValue: string;
  payloadJson: string;
  reset: boolean;
}

export class CrossLanguageAttributeBridge {
  private static readonly defaultFontScaleResetValue: number = 0.0;
  private static uiContextByRenderId: Map<number, UIContext> = new Map();
  private static pendingRequestsByRenderId: Map<number, PendingCrossLanguageAttributeRequest[]> = new Map();

  static setUIContext(renderId: number, context: UIContext): void {
    if (renderId < 0) {
      console.warn('[CrossLanguageAttributeBridge] setUIContext ignored: invalid renderId');
      return;
    }
    CrossLanguageAttributeBridge.uiContextByRenderId.set(renderId, context);
  }

  static clearUIContext(renderId: number): void {
    if (renderId < 0) {
      return;
    }
    CrossLanguageAttributeBridge.uiContextByRenderId.delete(renderId);
    CrossLanguageAttributeBridge.pendingRequestsByRenderId.delete(renderId);
  }

  static applyAttribute(
    renderId: number,
    componentId: string,
    nodeUniqueId: number,
    componentType: string,
    attributeName: string,
    floatValue: number,
    stringValue: string,
    payloadJson: string,
    reset: boolean
  ): void {
    const request: PendingCrossLanguageAttributeRequest = {
      renderId: renderId,
      componentId: componentId,
      nodeUniqueId: nodeUniqueId,
      componentType: componentType,
      attributeName: attributeName,
      floatValue: floatValue,
      stringValue: stringValue,
      payloadJson: payloadJson,
      reset: reset
    };
    CrossLanguageAttributeBridge.applyOrDefer(request);
  }

  private static applyOrDefer(request: PendingCrossLanguageAttributeRequest): void {
    if (CrossLanguageAttributeBridge.tryApplyAttribute(request, true)) {
      return;
    }
    CrossLanguageAttributeBridge.logRequest('applyAttribute deferred', request);
    CrossLanguageAttributeBridge.enqueuePendingRequest(request);
  }

  static flushPending(renderId: number): void {
    let pendingCount: number = CrossLanguageAttributeBridge.pendingRequestsByRenderId.get(renderId)?.length ?? 0;
    if (pendingCount > 0) {
      console.info(`[CrossLanguageAttributeBridge] flushPending: renderId=${renderId}, pendingCount=${pendingCount}`);
    }
    CrossLanguageAttributeBridge.replayPendingRequests(renderId);
  }

  private static tryApplyAttribute(request: PendingCrossLanguageAttributeRequest, logOnFailure: boolean): boolean {
    let renderId: number = request.renderId;
    let componentId: string = request.componentId;
    let componentType: string = request.componentType;
    let attributeName: string = request.attributeName;
    let floatValue: number = request.floatValue;
    let stringValue: string = request.stringValue;
    let payloadJson: string = request.payloadJson;
    let reset: boolean = request.reset;
    let ctx = CrossLanguageAttributeBridge.uiContextByRenderId.get(renderId);
    if (ctx === undefined) {
      if (logOnFailure) {
        console.warn(`[CrossLanguageAttributeBridge] applyAttribute: UIContext not set for renderId=${renderId}`);
      }
      return false;
    }

    let frameNode: FrameNode | null = CrossLanguageAttributeBridge.findFrameNode(ctx, request, logOnFailure);
    if (frameNode === null || frameNode === undefined) {
      return false;
    }

    // ── Common attributes (CommonMethod, no component-specific typeNode needed) ──
    // Must be handled BEFORE component-type dispatch so that universal attributes
    // like constraintSize are not short-circuited by early returns.
    if (attributeName === 'constraintSize') {
      if (payloadJson.length === 0) {
        return true;
      }
      let options: ConstraintSizeOptions = {};
      try {
        options = JSON.parse(payloadJson) as ConstraintSizeOptions;
      } catch (e) {
        console.warn(
          `[CrossLanguageAttributeBridge] applyAttribute: constraintSize parse failed, payload=${payloadJson}`
        );
        return true;
      }
      // constraintSize is a CommonMethod method. FrameNode.commonAttribute provides
      // access to CommonAttribute (CommonMethod) without needing component-specific type dispatch.
      frameNode.commonAttribute.constraintSize(options);
      return true;
    }

    if (attributeName === 'backgroundImageSize') {
      if (payloadJson.length === 0) {
        return true;
      }
      let sizeOptions: SizeOptions = {};
      try {
        sizeOptions = JSON.parse(payloadJson) as SizeOptions;
      } catch (e) {
        console.warn(
          `[CrossLanguageAttributeBridge] applyAttribute: backgroundImageSize parse failed, payload=${payloadJson}`
        );
        return true;
      }
      // backgroundImageSize is a CommonMethod method. FrameNode.commonAttribute provides
      // access to CommonAttribute (CommonMethod) without needing component-specific type dispatch.
      frameNode.commonAttribute.backgroundImageSize(sizeOptions);
      return true;
    }

    if (attributeName === 'borderRadius') {
      if (payloadJson.length === 0) {
        return true;
      }
      let radius: Length | BorderRadiuses = 0;
      try {
        radius = JSON.parse(payloadJson) as Length | BorderRadiuses;
      } catch (e) {
        console.warn(
          `[CrossLanguageAttributeBridge] applyAttribute: borderRadius parse failed, payload=${payloadJson}`
        );
        return true;
      }
      if (componentType === 'Button') {
        let buttonAttr = typeNode.getAttribute(frameNode, 'Button');
        if (buttonAttr === null || buttonAttr === undefined) {
          if (logOnFailure) {
            console.warn(
              `[CrossLanguageAttributeBridge] applyAttribute: failed to get Button attribute for id=${componentId}`
            );
          }
          return false;
        }
        buttonAttr.type(ButtonType.Normal);
        buttonAttr.borderRadius(radius);
        return true;
      }
      frameNode.commonAttribute.borderRadius(radius);
      return true;
    }

    if (CrossLanguageAttributeBridge.tryApplyLayoutPolicy(frameNode, componentType, attributeName, payloadJson)) {
      return true;
    }

    // ── Component-specific attributes ──────────────────────────────────────────
    if (componentType === 'Text') {
      let textAttr = typeNode.getAttribute(frameNode, 'Text');
      if (textAttr === null || textAttr === undefined) {
        if (logOnFailure) {
          console.warn(
            `[CrossLanguageAttributeBridge] applyAttribute: failed to get Text attribute for id=${componentId}`
          );
        }
        return false;
      }
      if (attributeName === 'minFontScale') {
        textAttr.minFontScale(
          reset ? CrossLanguageAttributeBridge.defaultFontScaleResetValue : floatValue
        );
      } else if (attributeName === 'maxFontScale') {
        textAttr.maxFontScale(
          reset ? CrossLanguageAttributeBridge.defaultFontScaleResetValue : floatValue
        );
      }
      return true;
    }

    if (componentType === 'TextInput') {
      let textInputAttr = typeNode.getAttribute(frameNode, 'TextInput');
      if (textInputAttr === null || textInputAttr === undefined) {
        if (logOnFailure) {
          console.warn(
            `[CrossLanguageAttributeBridge] applyAttribute: failed to get TextInput attribute for id=${componentId}`
          );
        }
        return false;
      }
      if (attributeName === 'minFontScale') {
        textInputAttr.minFontScale(
          reset ? CrossLanguageAttributeBridge.defaultFontScaleResetValue : floatValue
        );
      } else if (attributeName === 'maxFontScale') {
        textInputAttr.maxFontScale(
          reset ? CrossLanguageAttributeBridge.defaultFontScaleResetValue : floatValue
        );
      }
      return true;
    }

    if (componentType === 'Button' && attributeName === 'buttonStyle') {
      let buttonAttr = typeNode.getAttribute(frameNode, 'Button');
      if (buttonAttr === null || buttonAttr === undefined) {
        if (logOnFailure) {
          console.warn(
            `[CrossLanguageAttributeBridge] applyAttribute: failed to get Button attribute for id=${componentId}`
          );
        }
        return false;
      }
      buttonAttr.buttonStyle(ButtonStyleMode.NORMAL);
      return true;
    }

    return true;
  }

  private static enqueuePendingRequest(request: PendingCrossLanguageAttributeRequest): void {
    let pendingRequests: PendingCrossLanguageAttributeRequest[] =
      CrossLanguageAttributeBridge.pendingRequestsByRenderId.get(request.renderId) ?? [];
    for (let index = 0; index < pendingRequests.length; index++) {
      let pendingRequest: PendingCrossLanguageAttributeRequest = pendingRequests[index];
      if (pendingRequest.componentId === request.componentId &&
        pendingRequest.componentType === request.componentType &&
        pendingRequest.attributeName === request.attributeName) {
        pendingRequests[index] = request;
        CrossLanguageAttributeBridge.pendingRequestsByRenderId.set(request.renderId, pendingRequests);
        return;
      }
    }
    pendingRequests.push(request);
    CrossLanguageAttributeBridge.pendingRequestsByRenderId.set(request.renderId, pendingRequests);
  }

  private static findFrameNode(
    ctx: UIContext,
    request: PendingCrossLanguageAttributeRequest,
    logOnFailure: boolean
  ): FrameNode | null {
    if (request.nodeUniqueId > 0) {
      let uniqueFrameNode: FrameNode | null = ctx.getFrameNodeByUniqueId(request.nodeUniqueId);
      if (uniqueFrameNode !== null && uniqueFrameNode !== undefined) {
        return uniqueFrameNode;
      }
      if (logOnFailure) {
        console.warn(
          `[CrossLanguageAttributeBridge] applyAttribute: frameNode not found for ` +
            `uniqueId=${request.nodeUniqueId}, id=${request.componentId}`
        );
      }
    }

    let attachedFrameNode: FrameNode | null = ctx.getAttachedFrameNodeById(request.componentId);
    if (attachedFrameNode === null || attachedFrameNode === undefined) {
      if (logOnFailure) {
        console.warn(
          `[CrossLanguageAttributeBridge] applyAttribute: attached frameNode not found for id=${request.componentId}`
        );
      }
      return null;
    }
    return attachedFrameNode;
  }

  private static replayPendingRequests(renderId: number): void {
    let pendingRequests: PendingCrossLanguageAttributeRequest[] | undefined =
      CrossLanguageAttributeBridge.pendingRequestsByRenderId.get(renderId);
    if (pendingRequests === undefined) {
      return;
    }

    let remainingRequests: PendingCrossLanguageAttributeRequest[] = [];
    for (let index = 0; index < pendingRequests.length; index++) {
      let request: PendingCrossLanguageAttributeRequest = pendingRequests[index];
      if (CrossLanguageAttributeBridge.tryApplyAttribute(request, true)) {
        continue;
      }
      remainingRequests.push(request);
    }

    if (remainingRequests.length === 0) {
      CrossLanguageAttributeBridge.pendingRequestsByRenderId.delete(renderId);
      console.info(`[CrossLanguageAttributeBridge] replayPendingRequests: all applied for renderId=${renderId}`);
      return;
    }
    CrossLanguageAttributeBridge.pendingRequestsByRenderId.set(renderId, remainingRequests);
    console.warn(
      `[CrossLanguageAttributeBridge] replayPendingRequests: remaining=${remainingRequests.length}, ` +
        `renderId=${renderId}`
    );
  }

  private static logRequest(label: string, request: PendingCrossLanguageAttributeRequest): void {
    console.warn(
      `[CrossLanguageAttributeBridge] ${label}: renderId=${request.renderId}, id=${request.componentId}, ` +
        `uniqueId=${request.nodeUniqueId}, component=${request.componentType}, attribute=${request.attributeName}`
    );
  }

  private static tryApplyLayoutPolicy(frameNode: FrameNode, componentType: string, attributeName: string,
    payloadJson: string): boolean {
    if (attributeName !== 'layoutPolicy') {
      return false;
    }
    if (payloadJson.length === 0) {
      return true;
    }
    let payload: LayoutPolicyPayload = { axis: 'width', policy: 0 };
    try {
      payload = JSON.parse(payloadJson) as LayoutPolicyPayload;
    } catch (e) {
      console.warn(
        `[CrossLanguageAttributeBridge] applyAttribute: layoutPolicy parse failed, payload=${payloadJson}`
      );
      return true;
    }
    CrossLanguageAttributeBridge.applyLayoutPolicyAttribute(frameNode, componentType, payload.axis, payload.policy);
    return true;
  }

  // No-ops on an unknown policy value or an unsupported component type.
  private static applyLayoutPolicyAttribute(frameNode: FrameNode, componentType: string, axis: string,
    policy: number): void {
    let layoutPolicyValue: LayoutPolicy;
    if (policy === ArkUILayoutPolicyType.MATCH_PARENT) {
      layoutPolicyValue = LayoutPolicy.matchParent;
    } else if (policy === ArkUILayoutPolicyType.WRAP_CONTENT) {
      layoutPolicyValue = LayoutPolicy.wrapContent;
    } else if (policy === ArkUILayoutPolicyType.FIX_AT_IDEAL_SIZE) {
      layoutPolicyValue = LayoutPolicy.fixAtIdealSize;
    } else {
      console.warn(
        `[CrossLanguageAttributeBridge] applyAttribute: layoutPolicy unknown policy=${policy}`
      );
      return;
    }

    const attr = CrossLanguageAttributeBridge.getLayoutPolicyAttr(frameNode, componentType);
    if (attr === null || attr === undefined) {
      console.warn(
        `[CrossLanguageAttributeBridge] applyAttribute: layoutPolicy getAttribute returned null, ` +
          `componentType=${componentType}`
      );
      return;
    }
    if (axis === 'height') {
      attr.height(layoutPolicyValue);
    } else {
      attr.width(layoutPolicyValue);
    }
  }

  private static getLayoutPolicyAttr(frameNode: FrameNode, componentType: string) {
    switch (componentType) {
      case 'Button': return typeNode.getAttribute(frameNode, 'Button');
      case 'Checkbox': return typeNode.getAttribute(frameNode, 'Checkbox');
      case 'Radio': return typeNode.getAttribute(frameNode, 'Radio');
      case 'Slider': return typeNode.getAttribute(frameNode, 'Slider');
      case 'Toggle': return typeNode.getAttribute(frameNode, 'Toggle');
      case 'Progress': return typeNode.getAttribute(frameNode, 'Progress');
      case 'LoadingProgress': return typeNode.getAttribute(frameNode, 'LoadingProgress');
      case 'Image': return typeNode.getAttribute(frameNode, 'Image');
      case 'XComponent': return typeNode.getAttribute(frameNode, 'XComponent');
      case 'Column': return typeNode.getAttribute(frameNode, 'Column');
      case 'Row':
        return typeNode.getAttribute(frameNode, 'Flex') ?? typeNode.getAttribute(frameNode, 'Row');
      case 'Stack': return typeNode.getAttribute(frameNode, 'Stack');
      case 'Flex': return typeNode.getAttribute(frameNode, 'Flex');
      case 'RelativeContainer': return typeNode.getAttribute(frameNode, 'RelativeContainer');
      case 'Swiper': return typeNode.getAttribute(frameNode, 'Swiper');
      case 'Scroll': return typeNode.getAttribute(frameNode, 'Scroll');
      case 'List': return typeNode.getAttribute(frameNode, 'List');
      case 'ListItem': return typeNode.getAttribute(frameNode, 'ListItem');
      case 'ListItemGroup': return typeNode.getAttribute(frameNode, 'ListItemGroup');
      case 'WaterFlow': return typeNode.getAttribute(frameNode, 'WaterFlow');
      case 'FlowItem': return typeNode.getAttribute(frameNode, 'FlowItem');
      case 'Grid': return typeNode.getAttribute(frameNode, 'Grid');
      case 'GridItem': return typeNode.getAttribute(frameNode, 'GridItem');
      case 'Text': return typeNode.getAttribute(frameNode, 'Text');
      case 'TextInput': return typeNode.getAttribute(frameNode, 'TextInput');
      case 'TextArea': return typeNode.getAttribute(frameNode, 'TextArea');
      default: return null;
    }
  }

}
