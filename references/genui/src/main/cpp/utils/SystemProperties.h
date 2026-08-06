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

#ifndef A2UI_SYSTEM_PROPERTIES_H
#define A2UI_SYSTEM_PROPERTIES_H

#include <cstdint>

namespace NativeModule {

class SystemProperties final {
public:
    static SystemProperties& GetInstance();

    void SetApiVersion(int32_t apiVersion);
    int32_t GetApiVersion() const;

private:
    SystemProperties() = default;
    ~SystemProperties() = default;
    SystemProperties(const SystemProperties&) = delete;
    SystemProperties& operator=(const SystemProperties&) = delete;

    int32_t apiVersion_ = 0;
};

} // namespace NativeModule

#endif // A2UI_SYSTEM_PROPERTIES_H
