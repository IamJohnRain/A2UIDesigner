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

import { describe, expect, it } from '@ohos/hypium';
import { A2UIValueType } from '../main/ets/interface/Types';
import {
  CustomComponentDynamicValueResolver
} from '../main/ets/core/types/runtime/CustomComponentDynamicValueResolver';

export default function CustomComponentDynamicValueResolverUnitTest() {
  describe('CustomComponentDynamicValueResolverTest', () => {
    it('should_callback_once_with_undefined_for_undefined_property_value', 0, () => {
      let resolver: CustomComponentDynamicValueResolver = new CustomComponentDynamicValueResolver();
      let callbackCount: number = 0;
      let callbackValue: A2UIValueType = 'not-called';

      resolver.resolvePropertyValue<A2UIValueType>('value', undefined, (value: A2UIValueType): void => {
        callbackCount++;
        callbackValue = value;
      });

      expect(callbackCount).assertEqual(1);
      expect(callbackValue === undefined).assertTrue();
      expect(resolver.evaluateValue<A2UIValueType>(undefined) === undefined).assertTrue();
    });
  });
}
