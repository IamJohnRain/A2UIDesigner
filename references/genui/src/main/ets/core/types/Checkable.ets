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

import { A2UICheckRule } from './CheckRule'

export class A2UICheckable {
  public checks?: A2UICheckRule[]

  static schema(): string {
    return `{
      "description": "Validation rules for this component.",
      "type": "object",
      "properties": {
        "checks": {
          "type": "array",
          "description": "A list of validation checks.",
          "items": ${A2UICheckRule.schema()}
        }
      }
    }`
  }
}
