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
import { A2UIDynamicValue } from '../main/ets/core/types/DynamicValue';
import { DynamicValueResolver } from '../main/ets/core/types/runtime/DynamicValueResolver';
import { A2UIResolveContext } from '../main/ets/core/types/runtime/ResolveContext';
import { A2UIValueType } from '../main/ets/core/types/types';

interface NativeExpressionEngineMock {
  evaluateExpression: (expression: string, renderId?: number, surfaceId?: string) => ExpressionEvaluateResult;
}

interface ExpressionEnginePatchTarget {
  getEngine: () => NativeExpressionEngineMock | null;
}

export default function ExpressionEngineWithContextUnitTest() {
  describe('ExpressionEngineWithContextTest', () => {
    let originalGetEngine: () => NativeExpressionEngineMock | null =
      (ExpressionEngine as Object as ExpressionEnginePatchTarget).getEngine;
    let capturedExpression: string = '';
    let capturedRenderId: number | undefined = undefined;
    let capturedSurfaceId: string | undefined = undefined;
    let nativeResult: ExpressionEvaluateResult = {
      success: false,
      type: 'undefined'
    };
    let evaluateHandler: ((expression: string, renderId?: number, surfaceId?: string) =>
      ExpressionEvaluateResult) | undefined = undefined;

    const mockEngine: NativeExpressionEngineMock = {
      evaluateExpression: (
        expression: string,
        renderId?: number,
        surfaceId?: string
      ): ExpressionEvaluateResult => {
        capturedExpression = expression;
        capturedRenderId = renderId;
        capturedSurfaceId = surfaceId;
        if (evaluateHandler !== undefined) {
          return evaluateHandler(expression, renderId, surfaceId);
        }
        return nativeResult;
      }
    };

    beforeEach(() => {
      capturedExpression = '';
      capturedRenderId = undefined;
      capturedSurfaceId = undefined;
      nativeResult = {
        success: false,
        type: 'undefined'
      };
      evaluateHandler = undefined;
      (ExpressionEngine as Object as ExpressionEnginePatchTarget).getEngine = (): NativeExpressionEngineMock | null => {
        return mockEngine;
      };
    });

    afterEach(() => {
      (ExpressionEngine as Object as ExpressionEnginePatchTarget).getEngine = originalGetEngine;
    });

    it('should_pass_expression_and_context_to_native_engine', 0, () => {
      nativeResult = {
        success: true,
        type: 'string',
        value: 'sm'
      };

      const result: ExpressionEvaluateResult =
        ExpressionEngine.evaluateWithContext('{{ $__widthBreakpoint }}', 100, 'surface-1');

      expect(capturedExpression).assertEqual('{{ $__widthBreakpoint }}');
      expect(capturedRenderId as number).assertEqual(100);
      expect(capturedSurfaceId as string).assertEqual('surface-1');
      expect(result.success).assertTrue();
      expect(result.type).assertEqual('string');
      expect(result.value as string).assertEqual('sm');
    });

    it('should_return_undefined_when_native_engine_unavailable', 0, () => {
      (ExpressionEngine as Object as ExpressionEnginePatchTarget).getEngine = (): NativeExpressionEngineMock | null => {
        return null;
      };

      const result: ExpressionEvaluateResult =
        ExpressionEngine.evaluateWithContext('{{ $__colorMode }}', 100, 'surface-1');

      expect(result.success).assertFalse();
      expect(result.type).assertEqual('undefined');
      expect(result.value === undefined).assertTrue();
    });

    it('should_return_undefined_for_non_string_expression', 0, () => {
      const nonString: Record<string, Object> = {};
      const result: ExpressionEvaluateResult =
        ExpressionEngine.evaluateWithContext(nonString as Object as string, 100, 'surface-1');

      expect(result.success).assertFalse();
      expect(result.type).assertEqual('undefined');
      expect(result.value === undefined).assertTrue();
    });

    it('should_return_native_number_result_from_context_evaluation', 0, () => {
      nativeResult = {
        success: true,
        type: 'number',
        value: 42
      };

      const result: ExpressionEvaluateResult =
        ExpressionEngine.evaluateWithContext('{{ $__dataModel.count }}', 200, 'surface-2');

      expect(capturedExpression).assertEqual('{{ $__dataModel.count }}');
      expect(capturedRenderId as number).assertEqual(200);
      expect(capturedSurfaceId as string).assertEqual('surface-2');
      expect(result.success).assertTrue();
      expect(result.type).assertEqual('number');
      expect(result.value as number).assertEqual(42);
    });

    it('should_return_undefined_when_native_result_is_malformed', 0, () => {
      const malformed: Record<string, Object> = {};
      nativeResult = malformed as Object as ExpressionEvaluateResult;

      const result: ExpressionEvaluateResult =
        ExpressionEngine.evaluateWithContext('{{ $__widthBreakpoint }}', 100, 'surface-1');

      expect(result.success).assertFalse();
      expect(result.type).assertEqual('undefined');
    });

    it('should_resolve_expressions_recursively_inside_object_and_array_dynamic_values', 0, () => {
      evaluateHandler = (expression: string): ExpressionEvaluateResult => {
        if (expression === '{{ 12 + 4 }}') {
          return {
            success: true,
            type: 'number',
            value: 16
          };
        }
        if (expression === "{{ 'Alpha' }}") {
          return {
            success: true,
            type: 'string',
            value: 'Alpha'
          };
        }
        return {
          success: false,
          type: 'undefined'
        };
      };

      let value: Record<string, Object> = new Object() as Record<string, Object>;
      let font: Record<string, Object> = new Object() as Record<string, Object>;
      font['size'] = '{{ 12 + 4 }}' as Object;
      let familyBinding: Record<string, Object> = new Object() as Record<string, Object>;
      familyBinding['path'] = '/fontFamily' as Object;
      font['family'] = familyBinding as Object;
      let option: Record<string, Object> = new Object() as Record<string, Object>;
      option['value'] = "{{ 'Alpha' }}" as Object;
      let optionList: Object[] = [option as Object];
      value['font'] = font as Object;
      value['options'] = optionList as Object;
      let dataModel: ESObject = new Object() as ESObject;
      dataModel['fontFamily'] = 'HarmonyOS Sans';
      let context: A2UIResolveContext = {
        renderId: 300,
        surfaceId: 'surface-recursive',
        componentId: 'select-recursive',
        dataModel: dataModel
      };

      let result = DynamicValueResolver.resolveValue(value as Object as A2UIDynamicValue, context);

      expect(result.success).assertTrue();
      let resolved: Record<string, Object> = result.value as Record<string, Object>;
      let resolvedFont: Record<string, Object> = resolved['font'] as Record<string, Object>;
      expect(resolvedFont['size'] as number).assertEqual(16);
      expect(resolvedFont['family'] as string).assertEqual('HarmonyOS Sans');
      let options: Object[] = resolved['options'] as Object[];
      let firstOption: Record<string, Object> = options[0] as Record<string, Object>;
      expect(firstOption['value'] as string).assertEqual('Alpha');
      expect(capturedRenderId as number).assertEqual(300);
      expect(capturedSurfaceId as string).assertEqual('surface-recursive');
    });

    it('should_resolve_nested_object_expressions_in_function_args', 0, () => {
      evaluateHandler = (expression: string): ExpressionEvaluateResult => {
        if (expression === '{{ 1 + 1 }}') {
          return {
            success: true,
            type: 'number',
            value: 2
          };
        }
        return {
          success: false,
          type: 'undefined'
        };
      };

      let nested: Record<string, Object> = new Object() as Record<string, Object>;
      nested['count'] = '{{ 1 + 1 }}' as Object;
      let valueArg: Record<string, Object> = new Object() as Record<string, Object>;
      valueArg['nested'] = nested as Object;
      let args: Record<string, Object> = new Object() as Record<string, Object>;
      args['value'] = valueArg as Object;
      let call: Record<string, Object> = new Object() as Record<string, Object>;
      call['call'] = 'setDataModel' as Object;
      call['args'] = args as Object;
      call['returnType'] = 'void' as Object;
      let context: A2UIResolveContext = {
        renderId: 301,
        surfaceId: 'surface-action',
        componentId: 'button-action',
        invokeFunction: (functionCall): A2UIValueType => functionCall.args as Object as A2UIValueType
      };

      let result = DynamicValueResolver.resolveValue(call as Object as A2UIDynamicValue, context);

      expect(result.success).assertTrue();
      let resolvedArgs: Record<string, Object> = result.value as Record<string, Object>;
      let resolvedValueArg: Record<string, Object> = resolvedArgs['value'] as Record<string, Object>;
      let resolvedNested: Record<string, Object> = resolvedValueArg['nested'] as Record<string, Object>;
      expect(resolvedNested['count'] as number).assertEqual(2);
      expect(capturedRenderId as number).assertEqual(301);
      expect(capturedSurfaceId as string).assertEqual('surface-action');
    });
  });
}
