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

#include <cstring>
#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "TestFixture.h"

using namespace NativeModule;

class MockNapiProviderTest : public A2UITest {
protected:
    napi_env env_ = reinterpret_cast<napi_env>(0x100);

    napi_value CreateInt32Value(int32_t value)
    {
        napi_value result = nullptr;
        mockNapiPtr_->CreateInt32(env_, value, &result);
        return result;
    }

    napi_value CreateStringValue(const std::string& value)
    {
        napi_value result = nullptr;
        mockNapiPtr_->CreateStringUtf8(env_, value.c_str(), value.size(), &result);
        return result;
    }

    napi_value CreateUint32Value(uint32_t value)
    {
        napi_value result = nullptr;
        mockNapiPtr_->CreateUint32(env_, value, &result);
        return result;
    }

    napi_value CreateBoolValue(bool value)
    {
        napi_value result = nullptr;
        mockNapiPtr_->CreateBoolean(env_, value, &result);
        return result;
    }

    std::string ReadStringValue(napi_value value)
    {
        size_t length = 0;
        mockNapiPtr_->GetValueStringUtf8(env_, value, nullptr, 0, &length);
        std::vector<char> buffer(length + 1, '\0');
        size_t copied = 0;
        mockNapiPtr_->GetValueStringUtf8(env_, value, buffer.data(), buffer.size(), &copied);
        return std::string(buffer.data(), copied);
    }

    double ReadDoubleValue(napi_value value)
    {
        double result = 0.0;
        EXPECT_EQ(mockNapiPtr_->GetValueDouble(env_, value, &result), napi_ok);
        return result;
    }

    int32_t ReadInt32Value(napi_value value)
    {
        int32_t result = 0;
        EXPECT_EQ(mockNapiPtr_->GetValueInt32(env_, value, &result), napi_ok);
        return result;
    }

    uint32_t ReadUint32Value(napi_value value)
    {
        uint32_t result = 0;
        EXPECT_EQ(mockNapiPtr_->GetValueUint32(env_, value, &result), napi_ok);
        return result;
    }

    bool ReadBoolValue(napi_value value)
    {
        bool result = false;
        EXPECT_EQ(mockNapiPtr_->GetValueBool(env_, value, &result), napi_ok);
        return result;
    }

    napi_valuetype ReadTypeofValue(napi_value value)
    {
        napi_valuetype result = napi_undefined;
        EXPECT_EQ(mockNapiPtr_->Typeof(env_, value, &result), napi_ok);
        return result;
    }
};

/**
 * @tc.name: MockNapiProviderTest001
 * @tc.desc: Verify the following MockNapiProvider behavior: return property names as array.
 * @tc.type: FUNC
 */
TEST_F(MockNapiProviderTest, MockNapiProviderTest001)
{
    /**
     * @tc.steps: step1. Prepare mock NAPI values or override status and invoke the target mock provider interface.
     * @tc.expected: The returned status, output values, and stored mock state match the expectation.
     */

    napi_value object = nullptr;
    mockNapiPtr_->CreateObject(env_, &object);
    mockNapiPtr_->SetNamedProperty(env_, object, "second", CreateInt32Value(2));
    mockNapiPtr_->SetNamedProperty(env_, object, "first", CreateInt32Value(1));
    napi_value keys = nullptr;
    mockNapiPtr_->GetPropertyNames(env_, object, &keys);
    bool isArray = false;
    mockNapiPtr_->IsArray(env_, keys, &isArray);
    EXPECT_TRUE(isArray);
    uint32_t length = 0;
    mockNapiPtr_->GetArrayLength(env_, keys, &length);
    ASSERT_EQ(length, 2u);
    napi_value key0 = nullptr;
    napi_value key1 = nullptr;
    mockNapiPtr_->GetElement(env_, keys, 0, &key0);
    mockNapiPtr_->GetElement(env_, keys, 1, &key1);
    EXPECT_EQ(ReadStringValue(key0), "first");
    EXPECT_EQ(ReadStringValue(key1), "second");
}

/**
 * @tc.name: MockNapiProviderTest002
 * @tc.desc: Verify the following MockNapiProvider behavior: expand array length when setting element.
 * @tc.type: FUNC
 */
TEST_F(MockNapiProviderTest, MockNapiProviderTest002)
{
    /**
     * @tc.steps: step1. Prepare mock NAPI values or override status and invoke the target mock provider interface.
     * @tc.expected: The returned status, output values, and stored mock state match the expectation.
     */

    napi_value array = nullptr;
    mockNapiPtr_->CreateArrayWithLength(env_, 0, &array);
    napi_value value = CreateInt32Value(42);
    mockNapiPtr_->SetElement(env_, array, 2, value);
    uint32_t length = 0;
    mockNapiPtr_->GetArrayLength(env_, array, &length);
    EXPECT_EQ(length, 3u);
    napi_value item = nullptr;
    mockNapiPtr_->GetElement(env_, array, 2, &item);
    int32_t parsedValue = 0;
    mockNapiPtr_->GetValueInt32(env_, item, &parsedValue);
    EXPECT_EQ(parsedValue, 42);
}

/**
 * @tc.name: MockNapiProviderTest003
 * @tc.desc: Verify the following MockNapiProvider behavior: report string length when buffer is null.
 * @tc.type: FUNC
 */
TEST_F(MockNapiProviderTest, MockNapiProviderTest003)
{
    /**
     * @tc.steps: step1. Prepare mock NAPI values or override status and invoke the target mock provider interface.
     * @tc.expected: The returned status, output values, and stored mock state match the expectation.
     */

    napi_value value = CreateStringValue("hello");
    size_t length = 0;
    mockNapiPtr_->GetValueStringUtf8(env_, value, nullptr, 0, &length);
    EXPECT_EQ(length, 5u);
    char buffer[3] = {};
    size_t copied = 0;
    mockNapiPtr_->GetValueStringUtf8(env_, value, buffer, sizeof(buffer), &copied);
    EXPECT_EQ(std::string(buffer), "he");
    EXPECT_EQ(copied, 2u);
}

/**
 * @tc.name: MockNapiProviderTest004
 * @tc.desc: Verify the following MockNapiProvider behavior: keep create double status override until reset.
 * @tc.type: FUNC
 */
TEST_F(MockNapiProviderTest, MockNapiProviderTest004)
{
    /**
     * @tc.steps: step1. Prepare mock NAPI values or override status and invoke the target mock provider interface.
     * @tc.expected: The returned status, output values, and stored mock state match the expectation.
     */

    napi_value createdValue = nullptr;
    EXPECT_EQ(mockNapiPtr_->CreateDouble(env_, 3.14, &createdValue), napi_ok);
    EXPECT_NE(createdValue, nullptr);
    EXPECT_DOUBLE_EQ(ReadDoubleValue(createdValue), 3.14);
    napi_value untouchedValue = reinterpret_cast<napi_value>(0x1234);
    mockNapiPtr_->SetCreateDoubleStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->CreateDouble(env_, 6.28, &untouchedValue), napi_generic_failure);
    EXPECT_EQ(untouchedValue, reinterpret_cast<napi_value>(0x1234));
    napi_value stillUntouchedValue = reinterpret_cast<napi_value>(0x5678);
    EXPECT_EQ(mockNapiPtr_->CreateDouble(env_, 9.42, &stillUntouchedValue), napi_generic_failure);
    EXPECT_EQ(stillUntouchedValue, reinterpret_cast<napi_value>(0x5678));
    mockNapiPtr_->ResetCreateDoubleStatus();
    napi_value nextCreatedValue = nullptr;
    EXPECT_EQ(mockNapiPtr_->CreateDouble(env_, 6.28, &nextCreatedValue), napi_ok);
    EXPECT_NE(nextCreatedValue, nullptr);
    EXPECT_DOUBLE_EQ(ReadDoubleValue(nextCreatedValue), 6.28);
    EXPECT_EQ(mockNapiPtr_->CreateDouble(env_, 1.23, nullptr), napi_ok);
}

/**
 * @tc.name: MockNapiProviderTest005
 * @tc.desc: Verify the following MockNapiProvider behavior: keep get value int32 status override until reset.
 * @tc.type: FUNC
 */
TEST_F(MockNapiProviderTest, MockNapiProviderTest005)
{
    /**
     * @tc.steps: step1. Prepare mock NAPI values or override status and invoke the target mock provider interface.
     * @tc.expected: The returned status, output values, and stored mock state match the expectation.
     */

    napi_value value = CreateInt32Value(42);
    EXPECT_EQ(ReadInt32Value(value), 42);
    int32_t parsedValue = 99;
    mockNapiPtr_->SetGetValueInt32Status(napi_number_expected);
    EXPECT_EQ(mockNapiPtr_->GetValueInt32(env_, value, &parsedValue), napi_number_expected);
    EXPECT_EQ(parsedValue, 99);
    EXPECT_EQ(mockNapiPtr_->GetValueInt32(env_, value, &parsedValue), napi_number_expected);
    EXPECT_EQ(parsedValue, 99);
    mockNapiPtr_->ResetGetValueInt32Status();
    EXPECT_EQ(mockNapiPtr_->GetValueInt32(env_, value, &parsedValue), napi_ok);
    EXPECT_EQ(parsedValue, 42);
}

/**
 * @tc.name: MockNapiProviderTest006
 * @tc.desc: Verify the following MockNapiProvider behavior: keep get value double status override until reset.
 * @tc.type: FUNC
 */
TEST_F(MockNapiProviderTest, MockNapiProviderTest006)
{
    /**
     * @tc.steps: step1. Prepare mock NAPI values or override status and invoke the target mock provider interface.
     * @tc.expected: The returned status, output values, and stored mock state match the expectation.
     */

    napi_value value = nullptr;
    ASSERT_EQ(mockNapiPtr_->CreateDouble(env_, 3.14, &value), napi_ok);
    EXPECT_DOUBLE_EQ(ReadDoubleValue(value), 3.14);
    double parsedValue = 9.99;
    mockNapiPtr_->SetGetValueDoubleStatus(napi_number_expected);
    EXPECT_EQ(mockNapiPtr_->GetValueDouble(env_, value, &parsedValue), napi_number_expected);
    EXPECT_DOUBLE_EQ(parsedValue, 9.99);
    EXPECT_EQ(mockNapiPtr_->GetValueDouble(env_, value, &parsedValue), napi_number_expected);
    EXPECT_DOUBLE_EQ(parsedValue, 9.99);
    mockNapiPtr_->ResetGetValueDoubleStatus();
    EXPECT_EQ(mockNapiPtr_->GetValueDouble(env_, value, &parsedValue), napi_ok);
    EXPECT_DOUBLE_EQ(parsedValue, 3.14);
}

/**
 * @tc.name: MockNapiProviderTest007
 * @tc.desc: Verify the following MockNapiProvider behavior: keep get value uint32 status override until reset.
 * @tc.type: FUNC
 */
TEST_F(MockNapiProviderTest, MockNapiProviderTest007)
{
    /**
     * @tc.steps: step1. Prepare mock NAPI values or override status and invoke the target mock provider interface.
     * @tc.expected: The returned status, output values, and stored mock state match the expectation.
     */

    napi_value value = CreateUint32Value(42);
    EXPECT_EQ(ReadUint32Value(value), 42u);
    uint32_t parsedValue = 99;
    mockNapiPtr_->SetGetValueUint32Status(napi_number_expected);
    EXPECT_EQ(mockNapiPtr_->GetValueUint32(env_, value, &parsedValue), napi_number_expected);
    EXPECT_EQ(parsedValue, 99u);
    EXPECT_EQ(mockNapiPtr_->GetValueUint32(env_, value, &parsedValue), napi_number_expected);
    EXPECT_EQ(parsedValue, 99u);
    mockNapiPtr_->ResetGetValueUint32Status();
    EXPECT_EQ(mockNapiPtr_->GetValueUint32(env_, value, &parsedValue), napi_ok);
    EXPECT_EQ(parsedValue, 42u);
}

/**
 * @tc.name: MockNapiProviderTest008
 * @tc.desc: Verify the following MockNapiProvider behavior: keep get value bool status override until reset.
 * @tc.type: FUNC
 */
TEST_F(MockNapiProviderTest, MockNapiProviderTest008)
{
    /**
     * @tc.steps: step1. Prepare mock NAPI values or override status and invoke the target mock provider interface.
     * @tc.expected: The returned status, output values, and stored mock state match the expectation.
     */

    napi_value value = CreateBoolValue(true);
    EXPECT_TRUE(ReadBoolValue(value));
    bool parsedValue = false;
    mockNapiPtr_->SetGetValueBoolStatus(napi_boolean_expected);
    EXPECT_EQ(mockNapiPtr_->GetValueBool(env_, value, &parsedValue), napi_boolean_expected);
    EXPECT_FALSE(parsedValue);
    parsedValue = true;
    EXPECT_EQ(mockNapiPtr_->GetValueBool(env_, value, &parsedValue), napi_boolean_expected);
    EXPECT_TRUE(parsedValue);
    mockNapiPtr_->ResetGetValueBoolStatus();
    parsedValue = false;
    EXPECT_EQ(mockNapiPtr_->GetValueBool(env_, value, &parsedValue), napi_ok);
    EXPECT_TRUE(parsedValue);
}

/**
 * @tc.name: MockNapiProviderTest009
 * @tc.desc: Verify the following MockNapiProvider behavior: keep get value string utf8 status override until reset.
 * @tc.type: FUNC
 */
TEST_F(MockNapiProviderTest, MockNapiProviderTest009)
{
    /**
     * @tc.steps: step1. Prepare mock NAPI values or override status and invoke the target mock provider interface.
     * @tc.expected: The returned status, output values, and stored mock state match the expectation.
     */

    napi_value value = CreateStringValue("hello");
    EXPECT_EQ(ReadStringValue(value), "hello");
    char buffer[8] = "keep";
    size_t copied = 99;
    mockNapiPtr_->SetGetValueStringUtf8Status(napi_string_expected);
    EXPECT_EQ(mockNapiPtr_->GetValueStringUtf8(env_, value, buffer, sizeof(buffer), &copied), napi_string_expected);
    EXPECT_EQ(std::string(buffer), "keep");
    EXPECT_EQ(copied, 99u);
    copied = 77;
    EXPECT_EQ(mockNapiPtr_->GetValueStringUtf8(env_, value, nullptr, 0, &copied), napi_string_expected);
    EXPECT_EQ(copied, 77u);
    mockNapiPtr_->ResetGetValueStringUtf8Status();
    copied = 0;
    EXPECT_EQ(mockNapiPtr_->GetValueStringUtf8(env_, value, nullptr, 0, &copied), napi_ok);
    EXPECT_EQ(copied, 5u);
    std::memset(buffer, 0, sizeof(buffer));
    copied = 0;
    EXPECT_EQ(mockNapiPtr_->GetValueStringUtf8(env_, value, buffer, sizeof(buffer), &copied), napi_ok);
    EXPECT_EQ(std::string(buffer), "hello");
    EXPECT_EQ(copied, 5u);
}

/**
 * @tc.name: MockNapiProviderTest010
 * @tc.desc: Verify the following MockNapiProvider behavior: keep typeof status override until reset.
 * @tc.type: FUNC
 */
TEST_F(MockNapiProviderTest, MockNapiProviderTest010)
{
    /**
     * @tc.steps: step1. Prepare mock NAPI values or override status and invoke the target mock provider interface.
     * @tc.expected: The returned status, output values, and stored mock state match the expectation.
     */

    napi_value value = CreateBoolValue(true);
    EXPECT_EQ(ReadTypeofValue(value), napi_boolean);
    napi_valuetype valueType = napi_number;
    mockNapiPtr_->SetTypeofStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->Typeof(env_, value, &valueType), napi_generic_failure);
    EXPECT_EQ(valueType, napi_number);
    valueType = napi_string;
    EXPECT_EQ(mockNapiPtr_->Typeof(env_, value, &valueType), napi_generic_failure);
    EXPECT_EQ(valueType, napi_string);
    mockNapiPtr_->ResetTypeofStatus();
    EXPECT_EQ(mockNapiPtr_->Typeof(env_, value, &valueType), napi_ok);
    EXPECT_EQ(valueType, napi_boolean);
}

/**
 * @tc.name: MockNapiProviderTest011
 * @tc.desc: Verify the following MockNapiProvider behavior: keep get named property status override until reset.
 * @tc.type: FUNC
 */
TEST_F(MockNapiProviderTest, MockNapiProviderTest011)
{
    /**
     * @tc.steps: step1. Prepare mock NAPI values or override status and invoke the target mock provider interface.
     * @tc.expected: The returned status, output values, and stored mock state match the expectation.
     */

    napi_value object = nullptr;
    ASSERT_EQ(mockNapiPtr_->CreateObject(env_, &object), napi_ok);
    napi_value propertyValue = CreateInt32Value(42);
    ASSERT_EQ(mockNapiPtr_->SetNamedProperty(env_, object, "value", propertyValue), napi_ok);
    napi_value result = nullptr;
    EXPECT_EQ(mockNapiPtr_->GetNamedProperty(env_, object, "value", &result), napi_ok);
    EXPECT_EQ(ReadInt32Value(result), 42);
    result = reinterpret_cast<napi_value>(0x1234);
    mockNapiPtr_->SetGetNamedPropertyStatus(napi_object_expected);
    EXPECT_EQ(mockNapiPtr_->GetNamedProperty(env_, object, "value", &result), napi_object_expected);
    EXPECT_EQ(result, reinterpret_cast<napi_value>(0x1234));
    result = reinterpret_cast<napi_value>(0x5678);
    EXPECT_EQ(mockNapiPtr_->GetNamedProperty(env_, object, "value", &result), napi_object_expected);
    EXPECT_EQ(result, reinterpret_cast<napi_value>(0x5678));
    mockNapiPtr_->ResetGetNamedPropertyStatus();
    result = nullptr;
    EXPECT_EQ(mockNapiPtr_->GetNamedProperty(env_, object, "value", &result), napi_ok);
    EXPECT_EQ(ReadInt32Value(result), 42);
}

/**
 * @tc.name: MockNapiProviderTest012
 * @tc.desc: Verify the following MockNapiProvider behavior: keep get element status override until reset.
 * @tc.type: FUNC
 */
TEST_F(MockNapiProviderTest, MockNapiProviderTest012)
{
    /**
     * @tc.steps: step1. Prepare mock NAPI values or override status and invoke the target mock provider interface.
     * @tc.expected: The returned status, output values, and stored mock state match the expectation.
     */

    napi_value array = nullptr;
    ASSERT_EQ(mockNapiPtr_->CreateArrayWithLength(env_, 0, &array), napi_ok);
    napi_value elementValue = CreateInt32Value(42);
    ASSERT_EQ(mockNapiPtr_->SetElement(env_, array, 0, elementValue), napi_ok);
    napi_value result = nullptr;
    EXPECT_EQ(mockNapiPtr_->GetElement(env_, array, 0, &result), napi_ok);
    EXPECT_EQ(ReadInt32Value(result), 42);
    result = reinterpret_cast<napi_value>(0x1234);
    mockNapiPtr_->SetGetElementStatus(napi_array_expected);
    EXPECT_EQ(mockNapiPtr_->GetElement(env_, array, 0, &result), napi_array_expected);
    EXPECT_EQ(result, reinterpret_cast<napi_value>(0x1234));
    result = reinterpret_cast<napi_value>(0x5678);
    EXPECT_EQ(mockNapiPtr_->GetElement(env_, array, 0, &result), napi_array_expected);
    EXPECT_EQ(result, reinterpret_cast<napi_value>(0x5678));
    mockNapiPtr_->ResetGetElementStatus();
    result = nullptr;
    EXPECT_EQ(mockNapiPtr_->GetElement(env_, array, 0, &result), napi_ok);
    EXPECT_EQ(ReadInt32Value(result), 42);
}

/**
 * @tc.name: MockNapiProviderTest013
 * @tc.desc: Verify the following MockNapiProvider behavior: keep get reference value status override until reset.
 * @tc.type: FUNC
 */
TEST_F(MockNapiProviderTest, MockNapiProviderTest013)
{
    /**
     * @tc.steps: step1. Prepare mock NAPI values or override status and invoke the target mock provider interface.
     * @tc.expected: The returned status, output values, and stored mock state match the expectation.
     */

    napi_value value = CreateInt32Value(42);
    napi_ref ref = nullptr;
    ASSERT_EQ(mockNapiPtr_->CreateReference(env_, value, 1, &ref), napi_ok);
    napi_value result = nullptr;
    EXPECT_EQ(mockNapiPtr_->GetReferenceValue(env_, ref, &result), napi_ok);
    EXPECT_EQ(ReadInt32Value(result), 42);
    result = reinterpret_cast<napi_value>(0x1234);
    mockNapiPtr_->SetGetReferenceValueStatus(napi_invalid_arg);
    EXPECT_EQ(mockNapiPtr_->GetReferenceValue(env_, ref, &result), napi_invalid_arg);
    EXPECT_EQ(result, reinterpret_cast<napi_value>(0x1234));
    result = reinterpret_cast<napi_value>(0x5678);
    EXPECT_EQ(mockNapiPtr_->GetReferenceValue(env_, ref, &result), napi_invalid_arg);
    EXPECT_EQ(result, reinterpret_cast<napi_value>(0x5678));
    mockNapiPtr_->ResetGetReferenceValueStatus();
    result = nullptr;
    EXPECT_EQ(mockNapiPtr_->GetReferenceValue(env_, ref, &result), napi_ok);
    EXPECT_EQ(ReadInt32Value(result), 42);
}

/**
 * @tc.name: MockNapiProviderTest014
 * @tc.desc: Verify the following MockNapiProvider behavior: keep call function status override until reset.
 * @tc.type: FUNC
 */
TEST_F(MockNapiProviderTest, MockNapiProviderTest014)
{
    /**
     * @tc.steps: step1. Prepare mock NAPI values or override status and invoke the target mock provider interface.
     * @tc.expected: The returned status, output values, and stored mock state match the expectation.
     */

    napi_value functionValue = nullptr;
    ASSERT_EQ(mockNapiPtr_->CreateFunction(env_, "fn", 2, nullptr, nullptr, &functionValue), napi_ok);
    napi_value result = nullptr;
    EXPECT_EQ(mockNapiPtr_->CallFunction(env_, nullptr, functionValue, 0, nullptr, &result), napi_ok);
    EXPECT_NE(result, nullptr);
    result = reinterpret_cast<napi_value>(0x1234);
    mockNapiPtr_->SetCallFunctionStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->CallFunction(env_, nullptr, functionValue, 0, nullptr, &result), napi_generic_failure);
    EXPECT_EQ(result, reinterpret_cast<napi_value>(0x1234));
    result = reinterpret_cast<napi_value>(0x5678);
    EXPECT_EQ(mockNapiPtr_->CallFunction(env_, nullptr, functionValue, 0, nullptr, &result), napi_generic_failure);
    EXPECT_EQ(result, reinterpret_cast<napi_value>(0x5678));
    mockNapiPtr_->ResetCallFunctionStatus();
    result = nullptr;
    EXPECT_EQ(mockNapiPtr_->CallFunction(env_, nullptr, functionValue, 0, nullptr, &result), napi_ok);
    EXPECT_NE(result, nullptr);
}

/**
 * @tc.name: MockNapiProviderTest015
 * @tc.desc: Verify the following MockNapiProvider behavior: keep get callback info status override until reset.
 * @tc.type: FUNC
 */
TEST_F(MockNapiProviderTest, MockNapiProviderTest015)
{
    /**
     * @tc.steps: step1. Prepare mock NAPI values or override status and invoke the target mock provider interface.
     * @tc.expected: The returned status, output values, and stored mock state match the expectation.
     */

    napi_value arg = CreateInt32Value(42);
    mockNapiPtr_->SetCallbackArgs({ arg });
    size_t argc = 1;
    napi_value argv[1] = { reinterpret_cast<napi_value>(0x1234) };
    mockNapiPtr_->SetGetCbInfoStatus(napi_invalid_arg);
    EXPECT_EQ(mockNapiPtr_->GetCbInfo(env_, nullptr, &argc, argv, nullptr, nullptr), napi_invalid_arg);
    EXPECT_EQ(argc, 1u);
    EXPECT_EQ(argv[0], reinterpret_cast<napi_value>(0x1234));
    EXPECT_EQ(mockNapiPtr_->GetCbInfo(env_, nullptr, &argc, argv, nullptr, nullptr), napi_invalid_arg);
    EXPECT_EQ(argc, 1u);
    EXPECT_EQ(argv[0], reinterpret_cast<napi_value>(0x1234));
    mockNapiPtr_->ResetGetCbInfoStatus();
    EXPECT_EQ(mockNapiPtr_->GetCbInfo(env_, nullptr, &argc, argv, nullptr, nullptr), napi_ok);
    EXPECT_EQ(argc, 1u);
    EXPECT_EQ(argv[0], arg);
}

/**
 * @tc.name: MockNapiProviderTest016
 * @tc.desc: Verify the following MockNapiProvider behavior: keep basic creation status overrides until reset.
 * @tc.type: FUNC
 */
TEST_F(MockNapiProviderTest, MockNapiProviderTest016)
{
    /**
     * @tc.steps: step1. Prepare mock NAPI values or override status and invoke the target mock provider interface.
     * @tc.expected: The returned status, output values, and stored mock state match the expectation.
     */

    napi_value value = reinterpret_cast<napi_value>(0x1234);
    mockNapiPtr_->SetGetGlobalStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->GetGlobal(env_, &value), napi_generic_failure);
    EXPECT_EQ(value, reinterpret_cast<napi_value>(0x1234));
    mockNapiPtr_->ResetGetGlobalStatus();
    EXPECT_EQ(mockNapiPtr_->GetGlobal(env_, &value), napi_ok);
    EXPECT_EQ(ReadTypeofValue(value), napi_object);
    value = reinterpret_cast<napi_value>(0x2234);
    mockNapiPtr_->SetGetUndefinedStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->GetUndefined(env_, &value), napi_generic_failure);
    EXPECT_EQ(value, reinterpret_cast<napi_value>(0x2234));
    mockNapiPtr_->ResetGetUndefinedStatus();
    EXPECT_EQ(mockNapiPtr_->GetUndefined(env_, &value), napi_ok);
    EXPECT_EQ(ReadTypeofValue(value), napi_undefined);
    value = reinterpret_cast<napi_value>(0x3234);
    mockNapiPtr_->SetGetNullStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->GetNull(env_, &value), napi_generic_failure);
    EXPECT_EQ(value, reinterpret_cast<napi_value>(0x3234));
    mockNapiPtr_->ResetGetNullStatus();
    EXPECT_EQ(mockNapiPtr_->GetNull(env_, &value), napi_ok);
    EXPECT_EQ(ReadTypeofValue(value), napi_null);
    value = reinterpret_cast<napi_value>(0x4234);
    mockNapiPtr_->SetCreateInt32Status(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->CreateInt32(env_, 42, &value), napi_generic_failure);
    EXPECT_EQ(value, reinterpret_cast<napi_value>(0x4234));
    mockNapiPtr_->ResetCreateInt32Status();
    EXPECT_EQ(mockNapiPtr_->CreateInt32(env_, 42, &value), napi_ok);
    EXPECT_EQ(ReadInt32Value(value), 42);
    value = reinterpret_cast<napi_value>(0x5234);
    mockNapiPtr_->SetCreateUint32Status(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->CreateUint32(env_, 42, &value), napi_generic_failure);
    EXPECT_EQ(value, reinterpret_cast<napi_value>(0x5234));
    mockNapiPtr_->ResetCreateUint32Status();
    EXPECT_EQ(mockNapiPtr_->CreateUint32(env_, 42, &value), napi_ok);
    EXPECT_EQ(ReadUint32Value(value), 42u);
    value = reinterpret_cast<napi_value>(0x6234);
    mockNapiPtr_->SetCreateBooleanStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->CreateBoolean(env_, true, &value), napi_generic_failure);
    EXPECT_EQ(value, reinterpret_cast<napi_value>(0x6234));
    mockNapiPtr_->ResetCreateBooleanStatus();
    EXPECT_EQ(mockNapiPtr_->CreateBoolean(env_, true, &value), napi_ok);
    EXPECT_TRUE(ReadBoolValue(value));
    value = reinterpret_cast<napi_value>(0x7234);
    mockNapiPtr_->SetCreateStringUtf8Status(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->CreateStringUtf8(env_, "hello", 5, &value), napi_generic_failure);
    EXPECT_EQ(value, reinterpret_cast<napi_value>(0x7234));
    mockNapiPtr_->ResetCreateStringUtf8Status();
    EXPECT_EQ(mockNapiPtr_->CreateStringUtf8(env_, "hello", 5, &value), napi_ok);
    EXPECT_EQ(ReadStringValue(value), "hello");
    value = reinterpret_cast<napi_value>(0x8234);
    mockNapiPtr_->SetCreateObjectStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->CreateObject(env_, &value), napi_generic_failure);
    EXPECT_EQ(value, reinterpret_cast<napi_value>(0x8234));
    mockNapiPtr_->ResetCreateObjectStatus();
    EXPECT_EQ(mockNapiPtr_->CreateObject(env_, &value), napi_ok);
    EXPECT_EQ(ReadTypeofValue(value), napi_object);
    value = reinterpret_cast<napi_value>(0x9234);
    mockNapiPtr_->SetCreateArrayWithLengthStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->CreateArrayWithLength(env_, 2, &value), napi_generic_failure);
    EXPECT_EQ(value, reinterpret_cast<napi_value>(0x9234));
    mockNapiPtr_->ResetCreateArrayWithLengthStatus();
    EXPECT_EQ(mockNapiPtr_->CreateArrayWithLength(env_, 2, &value), napi_ok);
    bool isArray = false;
    EXPECT_EQ(mockNapiPtr_->IsArray(env_, value, &isArray), napi_ok);
    EXPECT_TRUE(isArray);
    value = reinterpret_cast<napi_value>(0xa234);
    mockNapiPtr_->SetCreateFunctionStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->CreateFunction(env_, "fn", 2, nullptr, nullptr, &value), napi_generic_failure);
    EXPECT_EQ(value, reinterpret_cast<napi_value>(0xa234));
    mockNapiPtr_->ResetCreateFunctionStatus();
    EXPECT_EQ(mockNapiPtr_->CreateFunction(env_, "fn", 2, nullptr, nullptr, &value), napi_ok);
    EXPECT_EQ(ReadTypeofValue(value), napi_function);
    value = CreateInt32Value(42);
    napi_ref ref = reinterpret_cast<napi_ref>(0x1111);
    mockNapiPtr_->SetCreateReferenceStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->CreateReference(env_, value, 1, &ref), napi_generic_failure);
    EXPECT_EQ(ref, reinterpret_cast<napi_ref>(0x1111));
    mockNapiPtr_->ResetCreateReferenceStatus();
    EXPECT_EQ(mockNapiPtr_->CreateReference(env_, value, 1, &ref), napi_ok);
    napi_value refValue = nullptr;
    EXPECT_EQ(mockNapiPtr_->GetReferenceValue(env_, ref, &refValue), napi_ok);
    EXPECT_EQ(ReadInt32Value(refValue), 42);
    value = reinterpret_cast<napi_value>(0xb234);
    mockNapiPtr_->SetCreateErrorStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->CreateError(env_, nullptr, nullptr, &value), napi_generic_failure);
    EXPECT_EQ(value, reinterpret_cast<napi_value>(0xb234));
    mockNapiPtr_->ResetCreateErrorStatus();
    EXPECT_EQ(mockNapiPtr_->CreateError(env_, nullptr, nullptr, &value), napi_ok);
    EXPECT_NE(value, nullptr);
    value = reinterpret_cast<napi_value>(0xc234);
    mockNapiPtr_->SetGetBooleanStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->GetBoolean(env_, true, &value), napi_generic_failure);
    EXPECT_EQ(value, reinterpret_cast<napi_value>(0xc234));
    mockNapiPtr_->ResetGetBooleanStatus();
    EXPECT_EQ(mockNapiPtr_->GetBoolean(env_, true, &value), napi_ok);
    EXPECT_TRUE(ReadBoolValue(value));
}

/**
 * @tc.name: MockNapiProviderTest017
 * @tc.desc: Verify the following MockNapiProvider behavior: keep container and query status overrides until reset.
 * @tc.type: FUNC
 */
TEST_F(MockNapiProviderTest, MockNapiProviderTest017)
{
    /**
     * @tc.steps: step1. Prepare mock NAPI values or override status and invoke the target mock provider interface.
     * @tc.expected: The returned status, output values, and stored mock state match the expectation.
     */

    napi_value object = nullptr;
    ASSERT_EQ(mockNapiPtr_->CreateObject(env_, &object), napi_ok);
    napi_value propertyValue = CreateInt32Value(42);
    mockNapiPtr_->SetSetNamedPropertyStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->SetNamedProperty(env_, object, "value", propertyValue), napi_generic_failure);
    bool hasProperty = false;
    EXPECT_EQ(mockNapiPtr_->HasNamedProperty(env_, object, "value", &hasProperty), napi_ok);
    EXPECT_FALSE(hasProperty);
    mockNapiPtr_->ResetSetNamedPropertyStatus();
    EXPECT_EQ(mockNapiPtr_->SetNamedProperty(env_, object, "value", propertyValue), napi_ok);
    hasProperty = false;
    mockNapiPtr_->SetHasNamedPropertyStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->HasNamedProperty(env_, object, "value", &hasProperty), napi_generic_failure);
    EXPECT_FALSE(hasProperty);
    mockNapiPtr_->ResetHasNamedPropertyStatus();
    EXPECT_EQ(mockNapiPtr_->HasNamedProperty(env_, object, "value", &hasProperty), napi_ok);
    EXPECT_TRUE(hasProperty);
    napi_value keys = reinterpret_cast<napi_value>(0x1234);
    mockNapiPtr_->SetGetPropertyNamesStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->GetPropertyNames(env_, object, &keys), napi_generic_failure);
    EXPECT_EQ(keys, reinterpret_cast<napi_value>(0x1234));
    mockNapiPtr_->ResetGetPropertyNamesStatus();
    EXPECT_EQ(mockNapiPtr_->GetPropertyNames(env_, object, &keys), napi_ok);
    uint32_t keyCount = 0;
    EXPECT_EQ(mockNapiPtr_->GetArrayLength(env_, keys, &keyCount), napi_ok);
    EXPECT_EQ(keyCount, 1u);
    napi_value array = nullptr;
    ASSERT_EQ(mockNapiPtr_->CreateArrayWithLength(env_, 0, &array), napi_ok);
    napi_value elementValue = CreateInt32Value(7);
    mockNapiPtr_->SetSetElementStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->SetElement(env_, array, 0, elementValue), napi_generic_failure);
    napi_value elementResult = reinterpret_cast<napi_value>(0x2345);
    EXPECT_EQ(mockNapiPtr_->GetElement(env_, array, 0, &elementResult), napi_ok);
    EXPECT_EQ(elementResult, nullptr);
    mockNapiPtr_->ResetSetElementStatus();
    EXPECT_EQ(mockNapiPtr_->SetElement(env_, array, 0, elementValue), napi_ok);
    bool isArray = false;
    mockNapiPtr_->SetIsArrayStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->IsArray(env_, array, &isArray), napi_generic_failure);
    EXPECT_FALSE(isArray);
    mockNapiPtr_->ResetIsArrayStatus();
    EXPECT_EQ(mockNapiPtr_->IsArray(env_, array, &isArray), napi_ok);
    EXPECT_TRUE(isArray);
    uint32_t length = 99;
    mockNapiPtr_->SetGetArrayLengthStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->GetArrayLength(env_, array, &length), napi_generic_failure);
    EXPECT_EQ(length, 99u);
    mockNapiPtr_->ResetGetArrayLengthStatus();
    EXPECT_EQ(mockNapiPtr_->GetArrayLength(env_, array, &length), napi_ok);
    EXPECT_EQ(length, 1u);
}

/**
 * @tc.name: MockNapiProviderTest018
 * @tc.desc: Verify the following MockNapiProvider behavior: keep delete reference and scope status overrides until
 * reset.
 * @tc.type: FUNC
 */
TEST_F(MockNapiProviderTest, MockNapiProviderTest018)
{
    /**
     * @tc.steps: step1. Prepare mock NAPI values or override status and invoke the target mock provider interface.
     * @tc.expected: The returned status, output values, and stored mock state match the expectation.
     */

    napi_value value = CreateInt32Value(42);
    napi_ref ref = nullptr;
    ASSERT_EQ(mockNapiPtr_->CreateReference(env_, value, 1, &ref), napi_ok);
    mockNapiPtr_->SetDeleteReferenceStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->DeleteReference(env_, ref), napi_generic_failure);
    napi_value refValue = nullptr;
    EXPECT_EQ(mockNapiPtr_->GetReferenceValue(env_, ref, &refValue), napi_ok);
    EXPECT_EQ(ReadInt32Value(refValue), 42);
    mockNapiPtr_->ResetDeleteReferenceStatus();
    EXPECT_EQ(mockNapiPtr_->DeleteReference(env_, ref), napi_ok);
    refValue = reinterpret_cast<napi_value>(0x1234);
    EXPECT_EQ(mockNapiPtr_->GetReferenceValue(env_, ref, &refValue), napi_ok);
    EXPECT_EQ(refValue, nullptr);
    napi_handle_scope handleScope = reinterpret_cast<napi_handle_scope>(0x1111);
    mockNapiPtr_->SetOpenHandleScopeStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->OpenHandleScope(env_, &handleScope), napi_generic_failure);
    EXPECT_EQ(handleScope, reinterpret_cast<napi_handle_scope>(0x1111));
    mockNapiPtr_->ResetOpenHandleScopeStatus();
    EXPECT_EQ(mockNapiPtr_->OpenHandleScope(env_, &handleScope), napi_ok);
    EXPECT_EQ(handleScope, nullptr);
    mockNapiPtr_->SetCloseHandleScopeStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->CloseHandleScope(env_, handleScope), napi_generic_failure);
    mockNapiPtr_->ResetCloseHandleScopeStatus();
    EXPECT_EQ(mockNapiPtr_->CloseHandleScope(env_, handleScope), napi_ok);
    napi_escapable_handle_scope escapableScope = reinterpret_cast<napi_escapable_handle_scope>(0x2222);
    mockNapiPtr_->SetOpenEscapableHandleScopeStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->OpenEscapableHandleScope(env_, &escapableScope), napi_generic_failure);
    EXPECT_EQ(escapableScope, reinterpret_cast<napi_escapable_handle_scope>(0x2222));
    mockNapiPtr_->ResetOpenEscapableHandleScopeStatus();
    EXPECT_EQ(mockNapiPtr_->OpenEscapableHandleScope(env_, &escapableScope), napi_ok);
    EXPECT_EQ(escapableScope, nullptr);
    mockNapiPtr_->SetCloseEscapableHandleScopeStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->CloseEscapableHandleScope(env_, escapableScope), napi_generic_failure);
    mockNapiPtr_->ResetCloseEscapableHandleScopeStatus();
    EXPECT_EQ(mockNapiPtr_->CloseEscapableHandleScope(env_, escapableScope), napi_ok);
    napi_value escapedValue = reinterpret_cast<napi_value>(0x3333);
    mockNapiPtr_->SetEscapeHandleStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->EscapeHandle(env_, escapableScope, value, &escapedValue), napi_generic_failure);
    EXPECT_EQ(escapedValue, reinterpret_cast<napi_value>(0x3333));
    mockNapiPtr_->ResetEscapeHandleStatus();
    EXPECT_EQ(mockNapiPtr_->EscapeHandle(env_, escapableScope, value, &escapedValue), napi_ok);
    EXPECT_EQ(escapedValue, value);
    mockNapiPtr_->SetThrowStatus(napi_generic_failure);
    EXPECT_EQ(mockNapiPtr_->Throw(env_, value), napi_generic_failure);
    mockNapiPtr_->ResetThrowStatus();
    EXPECT_EQ(mockNapiPtr_->Throw(env_, value), napi_ok);
}
