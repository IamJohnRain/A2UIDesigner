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

#ifndef A2UI_ACTION_PARSER_H
#define A2UI_ACTION_PARSER_H

#include <cstdint>
#include <memory>
#include <string>

#include "../utils/JsonAdapter.h"
#include "ActionInfo.h"

namespace NativeModule {

struct ActionParseContext {
    int32_t renderId = 0;
    std::string surfaceId;
    std::string componentId;
};

class ActionParser final {
public:
    static std::shared_ptr<ActionInfo> Parse(const JsonValue& descriptor);
    static std::shared_ptr<ActionInfo> Parse(const JsonValue& descriptor, const ActionParseContext& context);

private:
    ActionParser() = default;
};

} // namespace NativeModule

#endif // A2UI_ACTION_PARSER_H
