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

import { afterEach, beforeEach, describe, expect, it } from '@ohos/hypium';
import {
  ExpressionEngine,
  ExpressionEvaluateResult
} from '../main/ets/core/expression/ExpressionEngine';

interface NativeExpressionEngine {
  evaluateExpression: (expression: string) => ExpressionEvaluateResult;
}

interface ExpressionEnginePatchTarget {
  getEngine: () => NativeExpressionEngine | null;
}

export default function ExpressionEngineUnitTest() {
  describe('ExpressionEngineTest', () => {
    let originalGetEngine: () => NativeExpressionEngine | null =
      (ExpressionEngine as Object as ExpressionEnginePatchTarget).getEngine;
    let capturedExpression: string = '';
    let nativeResult: ExpressionEvaluateResult = {
      success: false,
      type: 'undefined'
    };

    const mockEngine: NativeExpressionEngine = {
      evaluateExpression: (expression: string): ExpressionEvaluateResult => {
        capturedExpression = expression;
        return nativeResult;
      }
    };

    beforeEach(() => {
      capturedExpression = '';
      nativeResult = {
        success: false,
        type: 'undefined'
      };
      (ExpressionEngine as Object as ExpressionEnginePatchTarget).getEngine = (): NativeExpressionEngine | null => {
        return mockEngine;
      };
    });

    afterEach(() => {
      (ExpressionEngine as Object as ExpressionEnginePatchTarget).getEngine = originalGetEngine;
    });

    it('should_return_native_result_when_engine_available', 0, () => {
      nativeResult = {
        success: true,
        type: 'number',
        value: 3
      };

      const result: ExpressionEvaluateResult = ExpressionEngine.evaluate('{{ 1 + 2 }}');

      expect(capturedExpression).assertEqual('{{ 1 + 2 }}');
      expect(result.success).assertTrue();
      expect(result.type).assertEqual('number');
      expect(result.value as number).assertEqual(3);
    });

    it('should_return_undefined_result_when_native_engine_unavailable', 0, () => {
      (ExpressionEngine as Object as ExpressionEnginePatchTarget).getEngine = (): NativeExpressionEngine | null => {
        return null;
      };

      const result: ExpressionEvaluateResult = ExpressionEngine.evaluate('{{ 1 + 2 }}');

      expect(result.success).assertFalse();
      expect(result.type).assertEqual('undefined');
      expect(result.value === undefined).assertTrue();
    });
  });
}
