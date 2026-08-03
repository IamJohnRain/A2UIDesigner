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

#include "FunctionCallInfo.h"

namespace NativeModule {

FunctionCallInfo::FunctionCallInfo(
    const std::string& functionName, const JsonValue& args, const std::string& returnType)
    : functionName_(functionName), args_(args), returnType_(returnType)
{}

const std::string& FunctionCallInfo::GetFunctionName() const
{
    return functionName_;
}

const JsonValue& FunctionCallInfo::GetArgs() const
{
    return args_;
}

const std::string& FunctionCallInfo::GetReturnType() const
{
    return returnType_;
}

} // namespace NativeModule
