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

#ifndef A2UI_MOCK_NAPI_PROVIDER_H
#define A2UI_MOCK_NAPI_PROVIDER_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "INapiProvider.h"

namespace NativeModule {

class MockNapiProvider : public INapiProvider {
public:
    static std::unique_ptr<MockNapiProvider> Create();

    void Reset();
    void SetGetValueInt32Status(napi_status status);
    void ResetGetValueInt32Status();
    void SetGetValueBoolStatus(napi_status status);
    void ResetGetValueBoolStatus();
    void SetGetValueBoolFailOnCall(int32_t callIndex, napi_status status);
    void ResetGetValueBoolFailOnCall();
    void SetGetValueDoubleStatus(napi_status status);
    void ResetGetValueDoubleStatus();
    void SetGetValueDoubleFailOnCall(int32_t callIndex, napi_status status);
    void ResetGetValueDoubleFailOnCall();
    void SetGetValueUint32Status(napi_status status);
    void ResetGetValueUint32Status();
    void SetGetValueStringUtf8Status(napi_status status);
    void ResetGetValueStringUtf8Status();
    void SetTypeofStatus(napi_status status);
    void ResetTypeofStatus();
    void SetTypeofFailOnCall(int32_t callIndex, napi_status status);
    void ResetTypeofFailOnCall();
    void SetGetNamedPropertyStatus(napi_status status);
    void ResetGetNamedPropertyStatus();
    void SetGetNamedPropertyFailOnCall(int32_t callIndex, napi_status status);
    void ResetGetNamedPropertyFailOnCall();
    void SetGetElementStatus(napi_status status);
    void ResetGetElementStatus();
    void SetGetElementReturnNullOnCall(int32_t callIndex);
    void ResetGetElementReturnNullOnCall();
    void SetGetReferenceValueStatus(napi_status status);
    void ResetGetReferenceValueStatus();
    void SetCallFunctionStatus(napi_status status);
    void ResetCallFunctionStatus();
    void SetGetCbInfoStatus(napi_status status);
    void ResetGetCbInfoStatus();
    void SetGetGlobalStatus(napi_status status);
    void ResetGetGlobalStatus();
    void SetGetUndefinedStatus(napi_status status);
    void ResetGetUndefinedStatus();
    void SetGetNullStatus(napi_status status);
    void ResetGetNullStatus();
    void SetCreateInt32Status(napi_status status);
    void ResetCreateInt32Status();
    void SetCreateUint32Status(napi_status status);
    void ResetCreateUint32Status();
    void SetCreateBooleanStatus(napi_status status);
    void ResetCreateBooleanStatus();
    void SetCreateStringUtf8Status(napi_status status);
    void ResetCreateStringUtf8Status();
    void SetCreateObjectStatus(napi_status status);
    void ResetCreateObjectStatus();
    void SetCreateArrayWithLengthStatus(napi_status status);
    void ResetCreateArrayWithLengthStatus();
    void SetCreateFunctionStatus(napi_status status);
    void ResetCreateFunctionStatus();
    void SetCreateReferenceStatus(napi_status status);
    void ResetCreateReferenceStatus();
    void SetDeleteReferenceStatus(napi_status status);
    void ResetDeleteReferenceStatus();
    void SetSetNamedPropertyStatus(napi_status status);
    void ResetSetNamedPropertyStatus();
    void SetHasNamedPropertyStatus(napi_status status);
    void ResetHasNamedPropertyStatus();
    void SetGetPropertyNamesStatus(napi_status status);
    void ResetGetPropertyNamesStatus();
    void SetGetPropertyNamesReturnNullOnce();
    void ResetGetPropertyNamesReturnNullOnce();
    void SetSetElementStatus(napi_status status);
    void ResetSetElementStatus();
    void SetIsArrayStatus(napi_status status);
    void ResetIsArrayStatus();
    void SetGetArrayLengthStatus(napi_status status);
    void ResetGetArrayLengthStatus();
    void SetOpenHandleScopeStatus(napi_status status);
    void ResetOpenHandleScopeStatus();
    void SetCloseHandleScopeStatus(napi_status status);
    void ResetCloseHandleScopeStatus();
    void SetOpenEscapableHandleScopeStatus(napi_status status);
    void ResetOpenEscapableHandleScopeStatus();
    void SetCloseEscapableHandleScopeStatus(napi_status status);
    void ResetCloseEscapableHandleScopeStatus();
    void SetEscapeHandleStatus(napi_status status);
    void ResetEscapeHandleStatus();
    void SetCreateErrorStatus(napi_status status);
    void ResetCreateErrorStatus();
    void SetThrowStatus(napi_status status);
    void ResetThrowStatus();
    void SetGetBooleanStatus(napi_status status);
    void ResetGetBooleanStatus();
    void SetCreateDoubleStatus(napi_status status);
    void ResetCreateDoubleStatus();
    void SetThrowOnMethod(const std::string& methodName, int32_t callIndex = 1);
    void ResetThrowOnMethod();

    void SetCallbackArgs(const std::vector<napi_value>& args);
    std::vector<napi_value> pendingCallbackArgs_;

    int nextValueId_ = 1;
    napi_value AllocNapiValue();

    std::map<napi_ref, napi_value> refToValue_;
    int nextRefId_ = 1;

    std::map<napi_value, std::map<std::string, napi_value>> objectProperties_;
    std::map<napi_value, napi_valuetype> valueTypes_;
    std::map<napi_value, bool> boolValues_;
    std::map<napi_value, double> numberValues_;
    std::map<napi_value, std::string> stringValues_;
    std::map<napi_value, bool> isArrayFlags_;
    std::map<napi_value, uint32_t> arrayLengths_;
    std::map<napi_value, std::map<uint32_t, napi_value>> arrayElements_;
    size_t callFunctionCallCount_ = 0;
    napi_value lastCallFunctionRecv_ = nullptr;
    napi_value lastCallFunctionFunc_ = nullptr;
    std::vector<napi_value> lastCallFunctionArgs_;
    std::vector<std::vector<napi_value>> callFunctionArgsHistory_;

    napi_status GetCbInfo(napi_env env, napi_callback_info cbinfo, size_t* argc, napi_value* argv, napi_value* thisArg,
        void** data) override;
    napi_status GetValueInt32(napi_env env, napi_value value, int32_t* result) override;
    napi_status GetValueBool(napi_env env, napi_value value, bool* result) override;
    napi_status GetValueDouble(napi_env env, napi_value value, double* result) override;
    napi_status GetValueUint32(napi_env env, napi_value value, uint32_t* result) override;
    napi_status GetValueStringUtf8(napi_env env, napi_value value, char* buf, size_t bufsize, size_t* result) override;
    napi_status Typeof(napi_env env, napi_value value, napi_valuetype* result) override;
    napi_status GetGlobal(napi_env env, napi_value* result) override;
    napi_status GetUndefined(napi_env env, napi_value* result) override;
    napi_status GetNull(napi_env env, napi_value* result) override;

    napi_status CreateInt32(napi_env env, int32_t value, napi_value* result) override;
    napi_status CreateUint32(napi_env env, uint32_t value, napi_value* result) override;
    napi_status CreateDouble(napi_env env, double value, napi_value* result) override;
    napi_status CreateBoolean(napi_env env, bool value, napi_value* result) override;
    napi_status CreateStringUtf8(napi_env env, const char* str, size_t length, napi_value* result) override;
    napi_status CreateObject(napi_env env, napi_value* result) override;
    napi_status CreateArrayWithLength(napi_env env, size_t length, napi_value* result) override;
    napi_status CreateFunction(
        napi_env env, const char* utf8name, size_t length, napi_callback cb, void* data, napi_value* result) override;
    napi_status CreateReference(napi_env env, napi_value value, uint32_t initialRefcount, napi_ref* result) override;
    napi_status DeleteReference(napi_env env, napi_ref ref) override;
    napi_status GetReferenceValue(napi_env env, napi_ref ref, napi_value* result) override;

    napi_status SetNamedProperty(napi_env env, napi_value object, const char* key, napi_value value) override;
    napi_status GetNamedProperty(napi_env env, napi_value object, const char* key, napi_value* result) override;
    napi_status HasNamedProperty(napi_env env, napi_value object, const char* key, bool* result) override;
    napi_status GetPropertyNames(napi_env env, napi_value object, napi_value* result) override;
    napi_status SetElement(napi_env env, napi_value object, uint32_t index, napi_value value) override;
    napi_status GetElement(napi_env env, napi_value object, uint32_t index, napi_value* result) override;
    napi_status IsArray(napi_env env, napi_value value, bool* result) override;
    napi_status GetArrayLength(napi_env env, napi_value value, uint32_t* result) override;

    napi_status CallFunction(napi_env env, napi_value recv, napi_value func, size_t argc, const napi_value* argv,
        napi_value* result) override;

    napi_status OpenHandleScope(napi_env env, napi_handle_scope* result) override;
    napi_status CloseHandleScope(napi_env env, napi_handle_scope scope) override;
    napi_status OpenEscapableHandleScope(napi_env env, napi_escapable_handle_scope* result) override;
    napi_status CloseEscapableHandleScope(napi_env env, napi_escapable_handle_scope scope) override;
    napi_status EscapeHandle(
        napi_env env, napi_escapable_handle_scope scope, napi_value escapee, napi_value* result) override;

    napi_status CreateError(napi_env env, napi_value code, napi_value msg, napi_value* result) override;
    napi_status Throw(napi_env env, napi_value error) override;
    napi_status GetBoolean(napi_env env, bool value, napi_value* result) override;

private:
    MockNapiProvider();
    void ThrowIfRequested(const char* methodName);

    napi_status getValueInt32Status_ = napi_ok;
    napi_status getValueBoolStatus_ = napi_ok;
    int32_t getValueBoolCallCount_ = 0;
    int32_t getValueBoolFailOnCall_ = -1;
    napi_status getValueBoolFailOnCallStatus_ = napi_ok;
    napi_status getValueDoubleStatus_ = napi_ok;
    int32_t getValueDoubleCallCount_ = 0;
    int32_t getValueDoubleFailOnCall_ = -1;
    napi_status getValueDoubleFailOnCallStatus_ = napi_ok;
    napi_status getValueUint32Status_ = napi_ok;
    napi_status getValueStringUtf8Status_ = napi_ok;
    napi_status typeofStatus_ = napi_ok;
    int32_t typeofCallCount_ = 0;
    int32_t typeofFailOnCall_ = -1;
    napi_status typeofFailOnCallStatus_ = napi_ok;
    napi_status getNamedPropertyStatus_ = napi_ok;
    int32_t getNamedPropertyCallCount_ = 0;
    int32_t getNamedPropertyFailOnCall_ = -1;
    napi_status getNamedPropertyFailOnCallStatus_ = napi_ok;
    napi_status getElementStatus_ = napi_ok;
    int32_t getElementCallCount_ = 0;
    int32_t getElementReturnNullOnCall_ = -1;
    napi_status getReferenceValueStatus_ = napi_ok;
    napi_status callFunctionStatus_ = napi_ok;
    napi_status getCbInfoStatus_ = napi_ok;
    napi_status getGlobalStatus_ = napi_ok;
    napi_status getUndefinedStatus_ = napi_ok;
    napi_status getNullStatus_ = napi_ok;
    napi_status createInt32Status_ = napi_ok;
    napi_status createUint32Status_ = napi_ok;
    napi_status createBooleanStatus_ = napi_ok;
    napi_status createStringUtf8Status_ = napi_ok;
    napi_status createObjectStatus_ = napi_ok;
    napi_status createArrayWithLengthStatus_ = napi_ok;
    napi_status createFunctionStatus_ = napi_ok;
    napi_status createReferenceStatus_ = napi_ok;
    napi_status deleteReferenceStatus_ = napi_ok;
    napi_status setNamedPropertyStatus_ = napi_ok;
    napi_status hasNamedPropertyStatus_ = napi_ok;
    napi_status getPropertyNamesStatus_ = napi_ok;
    bool getPropertyNamesReturnNullOnce_ = false;
    napi_status setElementStatus_ = napi_ok;
    napi_status isArrayStatus_ = napi_ok;
    napi_status getArrayLengthStatus_ = napi_ok;
    napi_status openHandleScopeStatus_ = napi_ok;
    napi_status closeHandleScopeStatus_ = napi_ok;
    napi_status openEscapableHandleScopeStatus_ = napi_ok;
    napi_status closeEscapableHandleScopeStatus_ = napi_ok;
    napi_status escapeHandleStatus_ = napi_ok;
    napi_status createErrorStatus_ = napi_ok;
    napi_status throwStatus_ = napi_ok;
    napi_status getBooleanStatus_ = napi_ok;
    napi_status createDoubleStatus_ = napi_ok;
    std::string throwMethodName_;
    int32_t throwMethodCallIndex_ = -1;
    std::map<std::string, int32_t> throwMethodCallCounts_;
};

} // namespace NativeModule

#endif // A2UI_MOCK_NAPI_PROVIDER_H
