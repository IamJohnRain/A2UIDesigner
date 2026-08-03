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

export type ExpressionResultType = 'string' | 'number' | 'boolean' | 'undefined';

export interface ExpressionEvaluateResult {
  success: boolean;
  type: ExpressionResultType;
  value?: string | number | boolean;
}

interface NativeExpressionEngine {
  evaluateExpression: (expression: string, renderId?: number, surfaceId?: string) => ExpressionEvaluateResult;
}

export class ExpressionEngine {
  private static undefinedResult(): ExpressionEvaluateResult {
    return {
      success: false,
      type: 'undefined'
    };
  }

  private static getEngine(): NativeExpressionEngine | null {
    let engineObject = nativeEngine as Object | undefined;
    if (engineObject === undefined || engineObject === null) {
      return null;
    }
    return engineObject as NativeExpressionEngine;
  }

  public static evaluate(expression: string): ExpressionEvaluateResult {
    if (typeof expression !== 'string') {
      return ExpressionEngine.undefinedResult();
    }
    let engine = ExpressionEngine.getEngine();
    if (engine === null) {
      console.warn('[ExpressionEngine] evaluate ignored: native engine unavailable');
      return ExpressionEngine.undefinedResult();
    }
    let result = engine.evaluateExpression(expression);
    if (result === undefined || result === null || typeof result.success !== 'boolean' ||
      typeof result.type !== 'string') {
      return ExpressionEngine.undefinedResult();
    }
    return result;
  }

  public static evaluateWithContext(
    expression: string, renderId: number, surfaceId: string
  ): ExpressionEvaluateResult {
    if (typeof expression !== 'string') {
      return ExpressionEngine.undefinedResult();
    }
    let engine = ExpressionEngine.getEngine();
    if (engine === null) {
      console.warn('[ExpressionEngine] evaluateWithContext ignored: native engine unavailable');
      return ExpressionEngine.undefinedResult();
    }
    let result = engine.evaluateExpression(expression, renderId, surfaceId);
    if (result === undefined || result === null || typeof result.success !== 'boolean' ||
      typeof result.type !== 'string') {
      return ExpressionEngine.undefinedResult();
    }
    return result;
  }
}
