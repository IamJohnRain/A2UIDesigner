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

#ifndef A2UI_NATIVE_FORMAT_DATE_FUNCTION_H
#define A2UI_NATIVE_FORMAT_DATE_FUNCTION_H

#include <string>

#include "NativeFunctionBase.h"

namespace NativeModule {

class NativeFormatDateFunction : public NativeFunctionBase {
public:
    struct DateTimeParts {
        int year = 0;
        int month = 1;
        int day = 1;
        int hour = 0;
        int minute = 0;
        int second = 0;
    };

    std::string GetName() const override;
    FunctionResult Execute(const JsonValue& resolvedArgs) override;

    static bool ParseISO8601(const std::string& iso, DateTimeParts& parts);
    static std::string ApplyPattern(const DateTimeParts& parts, const std::string& pattern);
};

} // namespace NativeModule

#endif // A2UI_NATIVE_FORMAT_DATE_FUNCTION_H
