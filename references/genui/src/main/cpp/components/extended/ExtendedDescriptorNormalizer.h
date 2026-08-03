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

#ifndef A2UI_EXTENDED_DESCRIPTOR_NORMALIZER_H
#define A2UI_EXTENDED_DESCRIPTOR_NORMALIZER_H

#include <memory>
#include <string>
#include <vector>

#include "utils/JsonAdapter.h"

namespace NativeModule {

struct DescriptorValidationIssue {
    std::string code;
    std::string message;
    std::string path;
};

struct NormalizedExtendedDescriptor {
    std::unique_ptr<JsonAdapter> adapter;
    JsonValue descriptor;
    JsonValue styles;
    std::vector<DescriptorValidationIssue> validationIssues;

    bool IsValid() const
    {
        return adapter != nullptr && descriptor.IsObject();
    }
};

class ExtendedDescriptorNormalizer final {
public:
    static NormalizedExtendedDescriptor Normalize(const JsonValue& descriptor);
};

} // namespace NativeModule

#endif // A2UI_EXTENDED_DESCRIPTOR_NORMALIZER_H
