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

#include "mock_napi_provider.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace NativeModule {

MockNapiProvider::MockNapiProvider() = default;

std::unique_ptr<MockNapiProvider> MockNapiProvider::Create()
{
    return std::unique_ptr<MockNapiProvider>(new MockNapiProvider());
}

void MockNapiProvider::Reset()
{
    nextValueId_ = 1;
    nextRefId_ = 1;
    getValueInt32Status_ = napi_ok;
    getValueBoolStatus_ = napi_ok;
    getValueBoolCallCount_ = 0;
    getValueBoolFailOnCall_ = -1;
    getValueBoolFailOnCallStatus_ = napi_ok;
    getValueDoubleStatus_ = napi_ok;
    getValueDoubleCallCount_ = 0;
    getValueDoubleFailOnCall_ = -1;
    getValueDoubleFailOnCallStatus_ = napi_ok;
    getValueUint32Status_ = napi_ok;
    getValueStringUtf8Status_ = napi_ok;
    typeofStatus_ = napi_ok;
    typeofCallCount_ = 0;
    typeofFailOnCall_ = -1;
    typeofFailOnCallStatus_ = napi_ok;
    getNamedPropertyStatus_ = napi_ok;
    getNamedPropertyCallCount_ = 0;
    getNamedPropertyFailOnCall_ = -1;
    getNamedPropertyFailOnCallStatus_ = napi_ok;
    getElementStatus_ = napi_ok;
    getElementCallCount_ = 0;
    getElementReturnNullOnCall_ = -1;
    getReferenceValueStatus_ = napi_ok;
    callFunctionStatus_ = napi_ok;
    getCbInfoStatus_ = napi_ok;
    getGlobalStatus_ = napi_ok;
    getUndefinedStatus_ = napi_ok;
    getNullStatus_ = napi_ok;
    createInt32Status_ = napi_ok;
    createUint32Status_ = napi_ok;
    createBooleanStatus_ = napi_ok;
    createStringUtf8Status_ = napi_ok;
    createObjectStatus_ = napi_ok;
    createArrayWithLengthStatus_ = napi_ok;
    createFunctionStatus_ = napi_ok;
    createReferenceStatus_ = napi_ok;
    deleteReferenceStatus_ = napi_ok;
    setNamedPropertyStatus_ = napi_ok;
    hasNamedPropertyStatus_ = napi_ok;
    getPropertyNamesStatus_ = napi_ok;
    getPropertyNamesReturnNullOnce_ = false;
    setElementStatus_ = napi_ok;
    isArrayStatus_ = napi_ok;
    getArrayLengthStatus_ = napi_ok;
    openHandleScopeStatus_ = napi_ok;
    closeHandleScopeStatus_ = napi_ok;
    openEscapableHandleScopeStatus_ = napi_ok;
    closeEscapableHandleScopeStatus_ = napi_ok;
    escapeHandleStatus_ = napi_ok;
    createErrorStatus_ = napi_ok;
    throwStatus_ = napi_ok;
    getBooleanStatus_ = napi_ok;
    createDoubleStatus_ = napi_ok;
    throwMethodName_.clear();
    throwMethodCallIndex_ = -1;
    throwMethodCallCounts_.clear();
    refToValue_.clear();
    objectProperties_.clear();
    valueTypes_.clear();
    boolValues_.clear();
    numberValues_.clear();
    stringValues_.clear();
    isArrayFlags_.clear();
    arrayLengths_.clear();
    arrayElements_.clear();
    pendingCallbackArgs_.clear();
    callFunctionCallCount_ = 0;
    lastCallFunctionRecv_ = nullptr;
    lastCallFunctionFunc_ = nullptr;
    lastCallFunctionArgs_.clear();
    callFunctionArgsHistory_.clear();
}

void MockNapiProvider::SetCallbackArgs(const std::vector<napi_value>& args)
{
    pendingCallbackArgs_ = args;
}

void MockNapiProvider::SetThrowOnMethod(const std::string& methodName, int32_t callIndex)
{
    throwMethodName_ = methodName;
    throwMethodCallIndex_ = callIndex;
    throwMethodCallCounts_.clear();
}

void MockNapiProvider::ResetThrowOnMethod()
{
    throwMethodName_.clear();
    throwMethodCallIndex_ = -1;
    throwMethodCallCounts_.clear();
}

void MockNapiProvider::ThrowIfRequested(const char* methodName)
{
    if (methodName == nullptr || throwMethodName_ != methodName) {
        return;
    }
    int32_t& callCount = throwMethodCallCounts_[throwMethodName_];
    ++callCount;
    if (throwMethodCallIndex_ <= 0 || callCount == throwMethodCallIndex_) {
        throw std::runtime_error(std::string("MockNapiProvider::") + methodName);
    }
}

void MockNapiProvider::SetGetValueInt32Status(napi_status status)
{
    getValueInt32Status_ = status;
}

void MockNapiProvider::ResetGetValueInt32Status()
{
    getValueInt32Status_ = napi_ok;
}

void MockNapiProvider::SetGetValueBoolStatus(napi_status status)
{
    getValueBoolStatus_ = status;
}

void MockNapiProvider::ResetGetValueBoolStatus()
{
    getValueBoolStatus_ = napi_ok;
}

void MockNapiProvider::SetGetValueBoolFailOnCall(int32_t callIndex, napi_status status)
{
    getValueBoolCallCount_ = 0;
    getValueBoolFailOnCall_ = callIndex;
    getValueBoolFailOnCallStatus_ = status;
}

void MockNapiProvider::ResetGetValueBoolFailOnCall()
{
    getValueBoolCallCount_ = 0;
    getValueBoolFailOnCall_ = -1;
    getValueBoolFailOnCallStatus_ = napi_ok;
}

void MockNapiProvider::SetGetValueDoubleStatus(napi_status status)
{
    getValueDoubleStatus_ = status;
}

void MockNapiProvider::ResetGetValueDoubleStatus()
{
    getValueDoubleStatus_ = napi_ok;
}

void MockNapiProvider::SetGetValueDoubleFailOnCall(int32_t callIndex, napi_status status)
{
    getValueDoubleCallCount_ = 0;
    getValueDoubleFailOnCall_ = callIndex;
    getValueDoubleFailOnCallStatus_ = status;
}

void MockNapiProvider::ResetGetValueDoubleFailOnCall()
{
    getValueDoubleCallCount_ = 0;
    getValueDoubleFailOnCall_ = -1;
    getValueDoubleFailOnCallStatus_ = napi_ok;
}

void MockNapiProvider::SetGetValueUint32Status(napi_status status)
{
    getValueUint32Status_ = status;
}

void MockNapiProvider::ResetGetValueUint32Status()
{
    getValueUint32Status_ = napi_ok;
}

void MockNapiProvider::SetGetValueStringUtf8Status(napi_status status)
{
    getValueStringUtf8Status_ = status;
}

void MockNapiProvider::ResetGetValueStringUtf8Status()
{
    getValueStringUtf8Status_ = napi_ok;
}

void MockNapiProvider::SetTypeofStatus(napi_status status)
{
    typeofStatus_ = status;
}

void MockNapiProvider::ResetTypeofStatus()
{
    typeofStatus_ = napi_ok;
}

void MockNapiProvider::SetTypeofFailOnCall(int32_t callIndex, napi_status status)
{
    typeofCallCount_ = 0;
    typeofFailOnCall_ = callIndex;
    typeofFailOnCallStatus_ = status;
}

void MockNapiProvider::ResetTypeofFailOnCall()
{
    typeofCallCount_ = 0;
    typeofFailOnCall_ = -1;
    typeofFailOnCallStatus_ = napi_ok;
}

void MockNapiProvider::SetGetNamedPropertyStatus(napi_status status)
{
    getNamedPropertyStatus_ = status;
}

void MockNapiProvider::ResetGetNamedPropertyStatus()
{
    getNamedPropertyStatus_ = napi_ok;
}

void MockNapiProvider::SetGetNamedPropertyFailOnCall(int32_t callIndex, napi_status status)
{
    getNamedPropertyCallCount_ = 0;
    getNamedPropertyFailOnCall_ = callIndex;
    getNamedPropertyFailOnCallStatus_ = status;
}

void MockNapiProvider::ResetGetNamedPropertyFailOnCall()
{
    getNamedPropertyCallCount_ = 0;
    getNamedPropertyFailOnCall_ = -1;
    getNamedPropertyFailOnCallStatus_ = napi_ok;
}

void MockNapiProvider::SetGetElementStatus(napi_status status)
{
    getElementStatus_ = status;
}

void MockNapiProvider::ResetGetElementStatus()
{
    getElementStatus_ = napi_ok;
}

void MockNapiProvider::SetGetElementReturnNullOnCall(int32_t callIndex)
{
    getElementCallCount_ = 0;
    getElementReturnNullOnCall_ = callIndex;
}

void MockNapiProvider::ResetGetElementReturnNullOnCall()
{
    getElementCallCount_ = 0;
    getElementReturnNullOnCall_ = -1;
}

void MockNapiProvider::SetGetReferenceValueStatus(napi_status status)
{
    getReferenceValueStatus_ = status;
}

void MockNapiProvider::ResetGetReferenceValueStatus()
{
    getReferenceValueStatus_ = napi_ok;
}

void MockNapiProvider::SetCallFunctionStatus(napi_status status)
{
    callFunctionStatus_ = status;
}

void MockNapiProvider::ResetCallFunctionStatus()
{
    callFunctionStatus_ = napi_ok;
}

void MockNapiProvider::SetGetCbInfoStatus(napi_status status)
{
    getCbInfoStatus_ = status;
}

void MockNapiProvider::ResetGetCbInfoStatus()
{
    getCbInfoStatus_ = napi_ok;
}

void MockNapiProvider::SetGetGlobalStatus(napi_status status)
{
    getGlobalStatus_ = status;
}

void MockNapiProvider::ResetGetGlobalStatus()
{
    getGlobalStatus_ = napi_ok;
}

void MockNapiProvider::SetGetUndefinedStatus(napi_status status)
{
    getUndefinedStatus_ = status;
}

void MockNapiProvider::ResetGetUndefinedStatus()
{
    getUndefinedStatus_ = napi_ok;
}

void MockNapiProvider::SetGetNullStatus(napi_status status)
{
    getNullStatus_ = status;
}

void MockNapiProvider::ResetGetNullStatus()
{
    getNullStatus_ = napi_ok;
}

void MockNapiProvider::SetCreateInt32Status(napi_status status)
{
    createInt32Status_ = status;
}

void MockNapiProvider::ResetCreateInt32Status()
{
    createInt32Status_ = napi_ok;
}

void MockNapiProvider::SetCreateUint32Status(napi_status status)
{
    createUint32Status_ = status;
}

void MockNapiProvider::ResetCreateUint32Status()
{
    createUint32Status_ = napi_ok;
}

void MockNapiProvider::SetCreateBooleanStatus(napi_status status)
{
    createBooleanStatus_ = status;
}

void MockNapiProvider::ResetCreateBooleanStatus()
{
    createBooleanStatus_ = napi_ok;
}

void MockNapiProvider::SetCreateStringUtf8Status(napi_status status)
{
    createStringUtf8Status_ = status;
}

void MockNapiProvider::ResetCreateStringUtf8Status()
{
    createStringUtf8Status_ = napi_ok;
}

void MockNapiProvider::SetCreateObjectStatus(napi_status status)
{
    createObjectStatus_ = status;
}

void MockNapiProvider::ResetCreateObjectStatus()
{
    createObjectStatus_ = napi_ok;
}

void MockNapiProvider::SetCreateArrayWithLengthStatus(napi_status status)
{
    createArrayWithLengthStatus_ = status;
}

void MockNapiProvider::ResetCreateArrayWithLengthStatus()
{
    createArrayWithLengthStatus_ = napi_ok;
}

void MockNapiProvider::SetCreateFunctionStatus(napi_status status)
{
    createFunctionStatus_ = status;
}

void MockNapiProvider::ResetCreateFunctionStatus()
{
    createFunctionStatus_ = napi_ok;
}

void MockNapiProvider::SetCreateReferenceStatus(napi_status status)
{
    createReferenceStatus_ = status;
}

void MockNapiProvider::ResetCreateReferenceStatus()
{
    createReferenceStatus_ = napi_ok;
}

void MockNapiProvider::SetDeleteReferenceStatus(napi_status status)
{
    deleteReferenceStatus_ = status;
}

void MockNapiProvider::ResetDeleteReferenceStatus()
{
    deleteReferenceStatus_ = napi_ok;
}

void MockNapiProvider::SetSetNamedPropertyStatus(napi_status status)
{
    setNamedPropertyStatus_ = status;
}

void MockNapiProvider::ResetSetNamedPropertyStatus()
{
    setNamedPropertyStatus_ = napi_ok;
}

void MockNapiProvider::SetHasNamedPropertyStatus(napi_status status)
{
    hasNamedPropertyStatus_ = status;
}

void MockNapiProvider::ResetHasNamedPropertyStatus()
{
    hasNamedPropertyStatus_ = napi_ok;
}

void MockNapiProvider::SetGetPropertyNamesStatus(napi_status status)
{
    getPropertyNamesStatus_ = status;
}

void MockNapiProvider::ResetGetPropertyNamesStatus()
{
    getPropertyNamesStatus_ = napi_ok;
}

void MockNapiProvider::SetGetPropertyNamesReturnNullOnce()
{
    getPropertyNamesReturnNullOnce_ = true;
}

void MockNapiProvider::ResetGetPropertyNamesReturnNullOnce()
{
    getPropertyNamesReturnNullOnce_ = false;
}

void MockNapiProvider::SetSetElementStatus(napi_status status)
{
    setElementStatus_ = status;
}

void MockNapiProvider::ResetSetElementStatus()
{
    setElementStatus_ = napi_ok;
}

void MockNapiProvider::SetIsArrayStatus(napi_status status)
{
    isArrayStatus_ = status;
}

void MockNapiProvider::ResetIsArrayStatus()
{
    isArrayStatus_ = napi_ok;
}

void MockNapiProvider::SetGetArrayLengthStatus(napi_status status)
{
    getArrayLengthStatus_ = status;
}

void MockNapiProvider::ResetGetArrayLengthStatus()
{
    getArrayLengthStatus_ = napi_ok;
}

void MockNapiProvider::SetOpenHandleScopeStatus(napi_status status)
{
    openHandleScopeStatus_ = status;
}

void MockNapiProvider::ResetOpenHandleScopeStatus()
{
    openHandleScopeStatus_ = napi_ok;
}

void MockNapiProvider::SetCloseHandleScopeStatus(napi_status status)
{
    closeHandleScopeStatus_ = status;
}

void MockNapiProvider::ResetCloseHandleScopeStatus()
{
    closeHandleScopeStatus_ = napi_ok;
}

void MockNapiProvider::SetOpenEscapableHandleScopeStatus(napi_status status)
{
    openEscapableHandleScopeStatus_ = status;
}

void MockNapiProvider::ResetOpenEscapableHandleScopeStatus()
{
    openEscapableHandleScopeStatus_ = napi_ok;
}

void MockNapiProvider::SetCloseEscapableHandleScopeStatus(napi_status status)
{
    closeEscapableHandleScopeStatus_ = status;
}

void MockNapiProvider::ResetCloseEscapableHandleScopeStatus()
{
    closeEscapableHandleScopeStatus_ = napi_ok;
}

void MockNapiProvider::SetEscapeHandleStatus(napi_status status)
{
    escapeHandleStatus_ = status;
}

void MockNapiProvider::ResetEscapeHandleStatus()
{
    escapeHandleStatus_ = napi_ok;
}

void MockNapiProvider::SetCreateErrorStatus(napi_status status)
{
    createErrorStatus_ = status;
}

void MockNapiProvider::ResetCreateErrorStatus()
{
    createErrorStatus_ = napi_ok;
}

void MockNapiProvider::SetThrowStatus(napi_status status)
{
    throwStatus_ = status;
}

void MockNapiProvider::ResetThrowStatus()
{
    throwStatus_ = napi_ok;
}

void MockNapiProvider::SetGetBooleanStatus(napi_status status)
{
    getBooleanStatus_ = status;
}

void MockNapiProvider::ResetGetBooleanStatus()
{
    getBooleanStatus_ = napi_ok;
}

void MockNapiProvider::SetCreateDoubleStatus(napi_status status)
{
    createDoubleStatus_ = status;
}

void MockNapiProvider::ResetCreateDoubleStatus()
{
    createDoubleStatus_ = napi_ok;
}

napi_value MockNapiProvider::AllocNapiValue()
{
    napi_value v = reinterpret_cast<napi_value>(static_cast<intptr_t>(nextValueId_++));
    return v;
}

napi_status MockNapiProvider::GetCbInfo(
    napi_env, napi_callback_info, size_t* argc, napi_value* argv, napi_value*, void**)
{
    ThrowIfRequested("GetCbInfo");
    if (getCbInfoStatus_ != napi_ok) {
        return getCbInfoStatus_;
    }

    if (argc && argv) {
        size_t copyCount = std::min(*argc, pendingCallbackArgs_.size());
        for (size_t i = 0; i < copyCount; ++i) {
            argv[i] = pendingCallbackArgs_[i];
        }
        for (size_t i = copyCount; i < *argc; ++i) {
            argv[i] = nullptr;
        }
        *argc = pendingCallbackArgs_.size();
    }
    return napi_ok;
}

napi_status MockNapiProvider::GetValueInt32(napi_env, napi_value value, int32_t* result)
{
    if (getValueInt32Status_ != napi_ok) {
        return getValueInt32Status_;
    }

    if (result) {
        auto it = numberValues_.find(value);
        *result = it != numberValues_.end() ? static_cast<int32_t>(it->second) : 0;
    }
    return napi_ok;
}

napi_status MockNapiProvider::GetValueBool(napi_env, napi_value value, bool* result)
{
    ++getValueBoolCallCount_;
    if (getValueBoolFailOnCall_ > 0 && getValueBoolCallCount_ == getValueBoolFailOnCall_) {
        return getValueBoolFailOnCallStatus_;
    }

    if (getValueBoolStatus_ != napi_ok) {
        return getValueBoolStatus_;
    }

    if (result) {
        auto it = boolValues_.find(value);
        *result = it != boolValues_.end() ? it->second : false;
    }
    return napi_ok;
}

napi_status MockNapiProvider::GetValueDouble(napi_env, napi_value value, double* result)
{
    ++getValueDoubleCallCount_;
    if (getValueDoubleFailOnCall_ > 0 && getValueDoubleCallCount_ == getValueDoubleFailOnCall_) {
        return getValueDoubleFailOnCallStatus_;
    }

    if (getValueDoubleStatus_ != napi_ok) {
        return getValueDoubleStatus_;
    }

    if (result) {
        auto it = numberValues_.find(value);
        *result = it != numberValues_.end() ? it->second : 0.0;
    }
    return napi_ok;
}

napi_status MockNapiProvider::GetValueUint32(napi_env, napi_value value, uint32_t* result)
{
    if (getValueUint32Status_ != napi_ok) {
        return getValueUint32Status_;
    }

    if (result) {
        auto it = numberValues_.find(value);
        *result = it != numberValues_.end() ? static_cast<uint32_t>(it->second) : 0;
    }
    return napi_ok;
}

napi_status MockNapiProvider::GetValueStringUtf8(napi_env, napi_value value, char* buf, size_t bufsize, size_t* result)
{
    if (getValueStringUtf8Status_ != napi_ok) {
        return getValueStringUtf8Status_;
    }

    auto it = stringValues_.find(value);
    if (it == stringValues_.end()) {
        if (result)
            *result = 0;
        return napi_ok;
    }
    const std::string& str = it->second;
    if (buf == nullptr || bufsize == 0) {
        if (result)
            *result = str.size();
    } else {
        size_t copyLen = str.size() < bufsize - 1 ? str.size() : bufsize - 1;
        std::memcpy(buf, str.c_str(), copyLen);
        buf[copyLen] = '\0';
        if (result)
            *result = copyLen;
    }
    return napi_ok;
}

napi_status MockNapiProvider::Typeof(napi_env, napi_value value, napi_valuetype* result)
{
    ThrowIfRequested("Typeof");
    ++typeofCallCount_;
    if (typeofFailOnCall_ > 0 && typeofCallCount_ == typeofFailOnCall_) {
        return typeofFailOnCallStatus_;
    }

    if (typeofStatus_ != napi_ok) {
        return typeofStatus_;
    }

    if (result) {
        auto it = valueTypes_.find(value);
        *result = it != valueTypes_.end() ? it->second : napi_undefined;
    }
    return napi_ok;
}

napi_status MockNapiProvider::GetGlobal(napi_env, napi_value* result)
{
    ThrowIfRequested("GetGlobal");
    if (getGlobalStatus_ != napi_ok) {
        return getGlobalStatus_;
    }

    if (result) {
        *result = AllocNapiValue();
        valueTypes_[*result] = napi_object;
    }
    return napi_ok;
}

napi_status MockNapiProvider::GetUndefined(napi_env, napi_value* result)
{
    if (getUndefinedStatus_ != napi_ok) {
        return getUndefinedStatus_;
    }

    if (result) {
        *result = AllocNapiValue();
        valueTypes_[*result] = napi_undefined;
    }
    return napi_ok;
}

napi_status MockNapiProvider::GetNull(napi_env, napi_value* result)
{
    if (getNullStatus_ != napi_ok) {
        return getNullStatus_;
    }

    if (result) {
        *result = AllocNapiValue();
        valueTypes_[*result] = napi_null;
    }
    return napi_ok;
}

napi_status MockNapiProvider::CreateInt32(napi_env, int32_t value, napi_value* result)
{
    ThrowIfRequested("CreateInt32");
    if (createInt32Status_ != napi_ok) {
        return createInt32Status_;
    }

    if (result) {
        *result = AllocNapiValue();
        numberValues_[*result] = static_cast<double>(value);
        valueTypes_[*result] = napi_number;
    }
    return napi_ok;
}

napi_status MockNapiProvider::CreateUint32(napi_env, uint32_t value, napi_value* result)
{
    if (createUint32Status_ != napi_ok) {
        return createUint32Status_;
    }

    if (result) {
        *result = AllocNapiValue();
        numberValues_[*result] = static_cast<double>(value);
        valueTypes_[*result] = napi_number;
    }
    return napi_ok;
}

napi_status MockNapiProvider::CreateDouble(napi_env, double value, napi_value* result)
{
    if (createDoubleStatus_ != napi_ok) {
        return createDoubleStatus_;
    }

    if (result) {
        *result = AllocNapiValue();
        numberValues_[*result] = value;
        valueTypes_[*result] = napi_number;
    }
    return napi_ok;
}

napi_status MockNapiProvider::CreateBoolean(napi_env, bool value, napi_value* result)
{
    if (createBooleanStatus_ != napi_ok) {
        return createBooleanStatus_;
    }

    if (result) {
        *result = AllocNapiValue();
        boolValues_[*result] = value;
        valueTypes_[*result] = napi_boolean;
    }
    return napi_ok;
}

napi_status MockNapiProvider::CreateStringUtf8(napi_env, const char* str, size_t, napi_value* result)
{
    ThrowIfRequested("CreateStringUtf8");
    if (createStringUtf8Status_ != napi_ok) {
        return createStringUtf8Status_;
    }

    if (result) {
        *result = AllocNapiValue();
        stringValues_[*result] = str ? std::string(str) : "";
        valueTypes_[*result] = napi_string;
    }
    return napi_ok;
}

napi_status MockNapiProvider::CreateObject(napi_env, napi_value* result)
{
    ThrowIfRequested("CreateObject");
    if (createObjectStatus_ != napi_ok) {
        return createObjectStatus_;
    }

    if (result) {
        *result = AllocNapiValue();
        valueTypes_[*result] = napi_object;
        objectProperties_[*result] = {};
    }
    return napi_ok;
}

napi_status MockNapiProvider::CreateArrayWithLength(napi_env, size_t length, napi_value* result)
{
    if (createArrayWithLengthStatus_ != napi_ok) {
        return createArrayWithLengthStatus_;
    }

    if (result) {
        *result = AllocNapiValue();
        isArrayFlags_[*result] = true;
        arrayLengths_[*result] = static_cast<uint32_t>(length);
        valueTypes_[*result] = napi_object;
        arrayElements_[*result] = {};
    }
    return napi_ok;
}

napi_status MockNapiProvider::CreateFunction(napi_env, const char*, size_t, napi_callback, void*, napi_value* result)
{
    if (createFunctionStatus_ != napi_ok) {
        return createFunctionStatus_;
    }

    if (result) {
        *result = AllocNapiValue();
        valueTypes_[*result] = napi_function;
    }
    return napi_ok;
}

napi_status MockNapiProvider::CreateReference(napi_env, napi_value value, uint32_t, napi_ref* result)
{
    ThrowIfRequested("CreateReference");
    if (createReferenceStatus_ != napi_ok) {
        return createReferenceStatus_;
    }

    if (result) {
        napi_ref ref = reinterpret_cast<napi_ref>(static_cast<intptr_t>(nextRefId_++));
        refToValue_[ref] = value;
        *result = ref;
    }
    return napi_ok;
}

napi_status MockNapiProvider::DeleteReference(napi_env, napi_ref ref)
{
    ThrowIfRequested("DeleteReference");
    if (deleteReferenceStatus_ != napi_ok) {
        return deleteReferenceStatus_;
    }

    refToValue_.erase(ref);
    return napi_ok;
}

napi_status MockNapiProvider::GetReferenceValue(napi_env, napi_ref ref, napi_value* result)
{
    ThrowIfRequested("GetReferenceValue");
    if (getReferenceValueStatus_ != napi_ok) {
        return getReferenceValueStatus_;
    }

    if (result) {
        auto it = refToValue_.find(ref);
        *result = it != refToValue_.end() ? it->second : nullptr;
    }
    return napi_ok;
}

napi_status MockNapiProvider::SetNamedProperty(napi_env, napi_value object, const char* key, napi_value value)
{
    ThrowIfRequested("SetNamedProperty");
    if (setNamedPropertyStatus_ != napi_ok) {
        return setNamedPropertyStatus_;
    }

    objectProperties_[object][std::string(key)] = value;
    return napi_ok;
}

napi_status MockNapiProvider::GetNamedProperty(napi_env, napi_value object, const char* key, napi_value* result)
{
    ++getNamedPropertyCallCount_;
    if (getNamedPropertyFailOnCall_ > 0 && getNamedPropertyCallCount_ == getNamedPropertyFailOnCall_) {
        return getNamedPropertyFailOnCallStatus_;
    }

    if (getNamedPropertyStatus_ != napi_ok) {
        return getNamedPropertyStatus_;
    }

    if (result) {
        auto objIt = objectProperties_.find(object);
        if (objIt != objectProperties_.end()) {
            auto propIt = objIt->second.find(std::string(key));
            *result = propIt != objIt->second.end() ? propIt->second : nullptr;
        } else {
            *result = nullptr;
        }
    }
    return napi_ok;
}

napi_status MockNapiProvider::HasNamedProperty(napi_env, napi_value object, const char* key, bool* result)
{
    if (hasNamedPropertyStatus_ != napi_ok) {
        return hasNamedPropertyStatus_;
    }

    if (result) {
        auto objIt = objectProperties_.find(object);
        if (objIt != objectProperties_.end()) {
            *result = objIt->second.find(std::string(key)) != objIt->second.end();
        } else {
            *result = false;
        }
    }
    return napi_ok;
}

napi_status MockNapiProvider::GetPropertyNames(napi_env, napi_value object, napi_value* result)
{
    if (getPropertyNamesStatus_ != napi_ok) {
        return getPropertyNamesStatus_;
    }

    if (result == nullptr) {
        return napi_ok;
    }

    if (getPropertyNamesReturnNullOnce_) {
        getPropertyNamesReturnNullOnce_ = false;
        *result = nullptr;
        return napi_ok;
    }

    *result = AllocNapiValue();
    isArrayFlags_[*result] = true;
    valueTypes_[*result] = napi_object;

    uint32_t index = 0;
    auto objIt = objectProperties_.find(object);
    if (objIt != objectProperties_.end()) {
        for (const auto& [key, _] : objIt->second) {
            napi_value keyValue = AllocNapiValue();
            stringValues_[keyValue] = key;
            valueTypes_[keyValue] = napi_string;
            arrayElements_[*result][index++] = keyValue;
        }
    }
    arrayLengths_[*result] = index;
    return napi_ok;
}

napi_status MockNapiProvider::SetElement(napi_env, napi_value object, uint32_t index, napi_value value)
{
    if (setElementStatus_ != napi_ok) {
        return setElementStatus_;
    }

    isArrayFlags_[object] = true;
    valueTypes_[object] = napi_object;
    arrayElements_[object][index] = value;
    uint32_t nextLength = index + 1;
    auto currentLength = arrayLengths_.find(object);
    if (currentLength == arrayLengths_.end() || currentLength->second < nextLength) {
        arrayLengths_[object] = nextLength;
    }
    return napi_ok;
}

napi_status MockNapiProvider::GetElement(napi_env, napi_value object, uint32_t index, napi_value* result)
{
    ++getElementCallCount_;

    if (getElementStatus_ != napi_ok) {
        return getElementStatus_;
    }

    if (getElementReturnNullOnCall_ > 0 && getElementCallCount_ == getElementReturnNullOnCall_) {
        if (result != nullptr) {
            *result = nullptr;
        }
        return napi_ok;
    }

    if (result) {
        auto arrIt = arrayElements_.find(object);
        if (arrIt != arrayElements_.end()) {
            auto elemIt = arrIt->second.find(index);
            *result = elemIt != arrIt->second.end() ? elemIt->second : nullptr;
        } else {
            *result = nullptr;
        }
    }
    return napi_ok;
}

napi_status MockNapiProvider::IsArray(napi_env, napi_value value, bool* result)
{
    if (isArrayStatus_ != napi_ok) {
        return isArrayStatus_;
    }

    if (result) {
        auto it = isArrayFlags_.find(value);
        *result = it != isArrayFlags_.end() ? it->second : false;
    }
    return napi_ok;
}

napi_status MockNapiProvider::GetArrayLength(napi_env, napi_value value, uint32_t* result)
{
    if (getArrayLengthStatus_ != napi_ok) {
        return getArrayLengthStatus_;
    }

    if (result) {
        auto it = arrayLengths_.find(value);
        *result = it != arrayLengths_.end() ? it->second : 0;
    }
    return napi_ok;
}

napi_status MockNapiProvider::CallFunction(
    napi_env, napi_value recv, napi_value func, size_t argc, const napi_value* argv, napi_value* result)
{
    ThrowIfRequested("CallFunction");
    if (callFunctionStatus_ != napi_ok) {
        return callFunctionStatus_;
    }

    ++callFunctionCallCount_;
    lastCallFunctionRecv_ = recv;
    lastCallFunctionFunc_ = func;
    lastCallFunctionArgs_.assign(argv, argv + argc);
    callFunctionArgsHistory_.push_back(lastCallFunctionArgs_);
    if (result)
        *result = AllocNapiValue();
    return napi_ok;
}

napi_status MockNapiProvider::OpenHandleScope(napi_env, napi_handle_scope* result)
{
    if (openHandleScopeStatus_ != napi_ok) {
        return openHandleScopeStatus_;
    }

    if (result)
        *result = nullptr;
    return napi_ok;
}

napi_status MockNapiProvider::CloseHandleScope(napi_env, napi_handle_scope)
{
    if (closeHandleScopeStatus_ != napi_ok) {
        return closeHandleScopeStatus_;
    }

    return napi_ok;
}

napi_status MockNapiProvider::OpenEscapableHandleScope(napi_env, napi_escapable_handle_scope* result)
{
    if (openEscapableHandleScopeStatus_ != napi_ok) {
        return openEscapableHandleScopeStatus_;
    }

    if (result)
        *result = nullptr;
    return napi_ok;
}

napi_status MockNapiProvider::CloseEscapableHandleScope(napi_env, napi_escapable_handle_scope)
{
    if (closeEscapableHandleScopeStatus_ != napi_ok) {
        return closeEscapableHandleScopeStatus_;
    }

    return napi_ok;
}

napi_status MockNapiProvider::EscapeHandle(
    napi_env, napi_escapable_handle_scope, napi_value escapee, napi_value* result)
{
    if (escapeHandleStatus_ != napi_ok) {
        return escapeHandleStatus_;
    }

    if (result)
        *result = escapee;
    return napi_ok;
}

napi_status MockNapiProvider::CreateError(napi_env, napi_value, napi_value, napi_value* result)
{
    if (createErrorStatus_ != napi_ok) {
        return createErrorStatus_;
    }

    if (result)
        *result = AllocNapiValue();
    return napi_ok;
}

napi_status MockNapiProvider::Throw(napi_env, napi_value)
{
    if (throwStatus_ != napi_ok) {
        return throwStatus_;
    }

    return napi_ok;
}

napi_status MockNapiProvider::GetBoolean(napi_env, bool value, napi_value* result)
{
    if (getBooleanStatus_ != napi_ok) {
        return getBooleanStatus_;
    }

    if (result) {
        *result = AllocNapiValue();
        boolValues_[*result] = value;
        valueTypes_[*result] = napi_boolean;
    }
    return napi_ok;
}

} // namespace NativeModule
