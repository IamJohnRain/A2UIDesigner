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

import { DynamicValue } from './DynamicValue'

export class A2UICheckRule {
  public condition: Record<string, object> = {};
  public message: string = ''

  static schema(): string {
    return `{
      "type": "object",
      "description": "A validation rule based on function call condition.",
      "properties": {
        "condition": {
          "type": "object",
          "description": "Check condition function definition.",
          "properties": {
            "call": {
              "type": "string",
              "description": "Builtin check function name.",
              "enum": ["required", "regex", "length", "numeric", "email"]
            },
            "args": {
              "type": "object",
              "description": "Arguments passed to check function.",
              "additionalProperties": {
                "anyOf": [
                  ${DynamicValue.schema()},
                  {
                    "type": "object",
                    "description": "Literal object argument."
                  }
                ]
              }
            },
            "returnType": {
              "type": "string",
              "description": "Expected return type.",
              "enum": ["boolean"],
              "default": "boolean"
            }
          },
          "required": ["call", "args"],
          "additionalProperties": false
        },
        "message": {
          "type": "string",
          "description": "Error message when validation fails."
        }
      },
      "required": ["condition", "message"],
      "additionalProperties": false
    }`
  }
}
