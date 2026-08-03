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

#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "components/extended/ExtendedDescriptorNormalizer.h"
#include "utils/JsonAdapter.h"

#include "TestFixture.h"

using namespace NativeModule;

namespace {

// =============================================================================
// ExtendedDescriptorNormalizer::Normalize - Branch Coverage
// =============================================================================
// Branches to cover:
//   1. adapter == nullptr (clone descriptor fails)
//   2. !descriptor.IsObject() (descriptor is not object)
//   3. styles IsValid && !IsObject (type mismatch)
//   4. styles !IsValid (styles absent)
//   5. Happy path: valid object with valid styles
// =============================================================================

class ExtendedDescriptorNormalizerTddTest : public A2UITest {
protected:
    // No special setup beyond A2UITest
};

/**
 * @tc.name: NormalizeInvalidJsonValue
 * @tc.desc: Passing an invalid JsonValue produces an invalid NormalizedExtendedDescriptor.
 *           Covers branch: adapter == nullptr after Clone(invalid).
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeInvalidJsonValue)
{
    JsonValue invalidValue;
    EXPECT_FALSE(invalidValue.IsValid());

    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(invalidValue);

    EXPECT_FALSE(result.IsValid());
    EXPECT_TRUE(result.validationIssues.empty());
}

/**
 * @tc.name: NormalizeNonObjectDescriptor
 * @tc.desc: A valid but non-object JsonValue (e.g. number/string/array) produces
 *           an invalid result. Covers branch: !descriptor.IsObject().
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeNonObjectDescriptor)
{
    {
        auto adapter = JsonAdapter::CreateNumber(42.0);
        ASSERT_NE(adapter, nullptr);
        NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
        EXPECT_FALSE(result.IsValid());
    }
    {
        auto adapter = JsonAdapter::CreateString("not_an_object");
        ASSERT_NE(adapter, nullptr);
        NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
        EXPECT_FALSE(result.IsValid());
    }
    {
        auto adapter = JsonAdapter::Parse("[1, 2, 3]");
        ASSERT_NE(adapter, nullptr);
        NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
        EXPECT_FALSE(result.IsValid());
    }
}

/**
 * @tc.name: NormalizeValidObjectWithNoStylesOrListeners
 * @tc.desc: A valid object with no styles or listeners produces a valid result
 *           with no validation issues. Covers branch: styles !IsValid.
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeValidObjectWithNoStylesOrListeners)
{
    auto adapter = JsonAdapter::Parse(R"({"id":"test","component":"Column"})");
    ASSERT_NE(adapter, nullptr);

    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());

    EXPECT_TRUE(result.IsValid());
    EXPECT_FALSE(result.styles.IsValid());
    EXPECT_TRUE(result.validationIssues.empty());
    EXPECT_EQ(result.descriptor.GetString("id", ""), "test");
    EXPECT_EQ(result.descriptor.GetString("component", ""), "Column");
}

/**
 * @tc.name: NormalizeValidObjectWithValidStyles
 * @tc.desc: A valid object with proper object-typed styles produces
 *           a valid result with no issues. Covers the happy path.
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeValidObjectWithValidStyles)
{
    auto adapter = JsonAdapter::Parse(R"({
        "id": "root",
        "component": "Text",
        "styles": {"fontSize": 16, "fontColor": "#FF000000"}
    })");
    ASSERT_NE(adapter, nullptr);

    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());

    EXPECT_TRUE(result.IsValid());
    EXPECT_TRUE(result.styles.IsValid());
    EXPECT_TRUE(result.styles.IsObject());
    EXPECT_TRUE(result.validationIssues.empty());
}

/**
 * @tc.name: NormalizeStylesTypeMismatch
 * @tc.desc: When styles is present but not an object (e.g. a string),
 *           a TYPE_MISMATCH validation issue is recorded.
 *           Covers branch: styles.IsValid() && !styles.IsObject().
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeStylesTypeMismatch)
{
    auto adapter = JsonAdapter::Parse(R"({
        "id": "root",
        "component": "Text",
        "styles": "invalid_not_object"
    })");
    ASSERT_NE(adapter, nullptr);

    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());

    EXPECT_TRUE(result.IsValid());
    ASSERT_EQ(result.validationIssues.size(), 1U);
    EXPECT_EQ(result.validationIssues[0].code, "ERROR_CODE_TYPE_MISMATCH");
    EXPECT_EQ(result.validationIssues[0].path, "styles");
    EXPECT_NE(result.validationIssues[0].message.find("styles"), std::string::npos);
}

/**
 * @tc.name: NormalizeValidObjectWithEmptyStyles
 * @tc.desc: An empty object for styles is valid (no type mismatch).
 *           Covers branch: styles.IsValid() && styles.IsObject().
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeValidObjectWithEmptyStyles)
{
    auto adapter = JsonAdapter::Parse(R"({
        "id": "root",
        "component": "Text",
        "styles": {}
    })");
    ASSERT_NE(adapter, nullptr);

    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());

    EXPECT_TRUE(result.IsValid());
    EXPECT_TRUE(result.styles.IsValid());
    EXPECT_TRUE(result.styles.IsObject());
    EXPECT_TRUE(result.validationIssues.empty());
}

/**
 * @tc.name: NormalizedExtendedDescriptorIsValidChecksAdapterAndDescriptor
 * @tc.desc: NormalizedExtendedDescriptor::IsValid() returns true only when
 *           both adapter is non-null and descriptor is object.
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizedExtendedDescriptorIsValidChecksAdapterAndDescriptor)
{
    NormalizedExtendedDescriptor desc;

    // Default state: no adapter, no descriptor object
    EXPECT_FALSE(desc.IsValid());

    // Set adapter only
    desc.adapter = JsonAdapter::CreateObject();
    ASSERT_NE(desc.adapter, nullptr);
    // descriptor is still invalid (default JsonValue)
    EXPECT_FALSE(desc.IsValid());

    // Set descriptor to valid object
    desc.descriptor = desc.adapter->GetRoot();
    EXPECT_TRUE(desc.IsValid());

    // Reset adapter to nullptr
    desc.adapter.reset();
    EXPECT_FALSE(desc.IsValid());
}

// =============================================================================
// Additional branch coverage tests
// =============================================================================

/**
 * @tc.name: NormalizeEmptyObjectDescriptor
 * @tc.desc: An empty JSON object produces a valid result with no validation issues.
 *           Covers branch: descriptor.IsObject()=true, no styles.
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeEmptyObjectDescriptor)
{
    auto adapter = JsonAdapter::Parse(R"({})");
    ASSERT_NE(adapter, nullptr);

    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());

    EXPECT_TRUE(result.IsValid());
    EXPECT_FALSE(result.styles.IsValid());
    EXPECT_TRUE(result.validationIssues.empty());
}

/**
 * @tc.name: NormalizeStylesTypeMismatchListenersAbsent
 * @tc.desc: When styles has a type mismatch but listeners is absent,
 *           only one validation issue is recorded.
 *           Covers branch: styles IsValid && !IsObject.
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeStylesTypeMismatchListenersAbsent)
{
    auto adapter = JsonAdapter::Parse(R"({
        "id": "root",
        "component": "Column",
        "styles": 42
    })");
    ASSERT_NE(adapter, nullptr);

    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());

    EXPECT_TRUE(result.IsValid());
    ASSERT_EQ(result.validationIssues.size(), 1U);
    EXPECT_EQ(result.validationIssues[0].code, "ERROR_CODE_TYPE_MISMATCH");
    EXPECT_EQ(result.validationIssues[0].path, "styles");
}

/**
 * @tc.name: NormalizeStylesArrayTypeMismatch
 * @tc.desc: When styles is an array, it triggers the type mismatch branch.
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeStylesArrayTypeMismatch)
{
    auto adapter = JsonAdapter::Parse(R"({
        "id": "root",
        "component": "Column",
        "styles": [1, 2, 3]
    })");
    ASSERT_NE(adapter, nullptr);

    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());

    EXPECT_TRUE(result.IsValid());
    ASSERT_EQ(result.validationIssues.size(), 1U);
    EXPECT_EQ(result.validationIssues[0].code, "ERROR_CODE_TYPE_MISMATCH");
    EXPECT_EQ(result.validationIssues[0].path, "styles");
}

/**
 * @tc.name: NormalizeBooleanDescriptor
 * @tc.desc: A boolean JsonValue produces an invalid result.
 *           Covers branch: !descriptor.IsObject().
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeBooleanDescriptor)
{
    auto adapter = JsonAdapter::Parse(R"(true)");
    ASSERT_NE(adapter, nullptr);
    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
    EXPECT_FALSE(result.IsValid());
}

/**
 * @tc.name: NormalizeNullDescriptor
 * @tc.desc: A null JsonValue produces an invalid result.
 *           Covers branch: !descriptor.IsObject().
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeNullDescriptor)
{
    auto adapter = JsonAdapter::Parse(R"(null)");
    ASSERT_NE(adapter, nullptr);
    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
    EXPECT_FALSE(result.IsValid());
}

/**
 * @tc.name: NormalizeStylesBoolTypeMismatch
 * @tc.desc: When styles is a boolean, it triggers the type mismatch branch.
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeStylesBoolTypeMismatch)
{
    auto adapter = JsonAdapter::Parse(R"({
        "id": "root",
        "component": "Column",
        "styles": true
    })");
    ASSERT_NE(adapter, nullptr);

    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());

    EXPECT_TRUE(result.IsValid());
    ASSERT_EQ(result.validationIssues.size(), 1U);
    EXPECT_EQ(result.validationIssues[0].code, "ERROR_CODE_TYPE_MISMATCH");
}

/**
 * @tc.name: NormalizeValidWithStylesAndExtraFields
 * @tc.desc: Valid descriptor with styles and extra fields.
 *           Exercises the LOG_A2UI with format strings containing componentId, component.
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeValidWithStylesAndExtraFields)
{
    auto adapter = JsonAdapter::Parse(R"({
        "id": "comp-123",
        "component": "Text",
        "styles": {"color": "red"},
        "children": [],
        "extra": "field"
    })");
    ASSERT_NE(adapter, nullptr);

    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());

    EXPECT_TRUE(result.IsValid());
    EXPECT_TRUE(result.styles.IsValid());
    EXPECT_TRUE(result.validationIssues.empty());
    EXPECT_EQ(result.descriptor.GetString("id", ""), "comp-123");
    EXPECT_EQ(result.descriptor.GetString("component", ""), "Text");
}

// =============================================================================
// COVERAGE GAP: Additional Normalize tests for full branch coverage
// =============================================================================

/**
 * @tc.name: NormalizeInvalidJsonValue_CloneReturnsNull
 * @tc.desc: Invalid JsonValue → JsonAdapter::Clone returns nullptr → adapter==nullptr path.
 *           Covers lines 34-37: adapter=nullptr → LOG_ERROR → return.
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeInvalidJsonValue_CloneReturnsNull)
{
    JsonValue invalidValue;
    EXPECT_FALSE(invalidValue.IsValid());
    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(invalidValue);
    EXPECT_FALSE(result.IsValid());
    EXPECT_EQ(result.adapter, nullptr);
    EXPECT_TRUE(result.validationIssues.empty());
}

/**
 * @tc.name: NormalizeNonObjectNumber_LogsNotObject
 * @tc.desc: Number value → clone succeeds → !IsObject() → LOG_WARN → reset + return.
 *           Covers lines 40-46: descriptor not object path.
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeNonObjectNumber_LogsNotObject)
{
    auto adapter = JsonAdapter::CreateNumber(42.0);
    ASSERT_NE(adapter, nullptr);
    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
    EXPECT_FALSE(result.IsValid());
    // adapter was set but then reset() at line 44
}

/**
 * @tc.name: NormalizeNonObjectString_LogsNotObject
 * @tc.desc: String value → clone succeeds → !IsObject() → LOG_WARN → reset + return.
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeNonObjectString_LogsNotObject)
{
    auto adapter = JsonAdapter::CreateString("not_an_object");
    ASSERT_NE(adapter, nullptr);
    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
    EXPECT_FALSE(result.IsValid());
}

/**
 * @tc.name: NormalizeNonObjectArray_LogsNotObject
 * @tc.desc: Array value → clone succeeds → !IsObject() → LOG_WARN → reset + return.
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeNonObjectArray_LogsNotObject)
{
    auto adapter = JsonAdapter::Parse(R"([1, 2, 3])");
    ASSERT_NE(adapter, nullptr);
    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
    EXPECT_FALSE(result.IsValid());
}

/**
 * @tc.name: NormalizeNonObjectBoolean_LogsNotObject
 * @tc.desc: Boolean value → clone succeeds → !IsObject() → LOG_WARN → reset + return.
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeNonObjectBoolean_LogsNotObject)
{
    auto adapter = JsonAdapter::Parse(R"(true)");
    ASSERT_NE(adapter, nullptr);
    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
    EXPECT_FALSE(result.IsValid());
}

/**
 * @tc.name: NormalizeNonObjectNull_LogsNotObject
 * @tc.desc: Null value → clone succeeds → !IsObject() → LOG_WARN → reset + return.
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeNonObjectNull_LogsNotObject)
{
    auto adapter = JsonAdapter::Parse(R"(null)");
    ASSERT_NE(adapter, nullptr);
    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
    EXPECT_FALSE(result.IsValid());
}

/**
 * @tc.name: NormalizeValidObjectStylesString_TypeMismatch
 * @tc.desc: Valid object with styles as string → type mismatch validation issue.
 *           Covers lines 52-57: styles IsValid && !IsObject → push validation issue + LOG_WARN.
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeValidObjectStylesString_TypeMismatch)
{
    auto adapter = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":"bad"})");
    ASSERT_NE(adapter, nullptr);
    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    ASSERT_EQ(result.validationIssues.size(), 1U);
    EXPECT_EQ(result.validationIssues[0].code, "ERROR_CODE_TYPE_MISMATCH");
    EXPECT_EQ(result.validationIssues[0].path, "styles");
}

/**
 * @tc.name: NormalizeValidObjectStylesNumber_TypeMismatch
 * @tc.desc: Valid object with styles as number → type mismatch.
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeValidObjectStylesNumber_TypeMismatch)
{
    auto adapter = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":42})");
    ASSERT_NE(adapter, nullptr);
    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    ASSERT_EQ(result.validationIssues.size(), 1U);
    EXPECT_EQ(result.validationIssues[0].code, "ERROR_CODE_TYPE_MISMATCH");
}

/**
 * @tc.name: NormalizeValidObjectStylesBool_TypeMismatch
 * @tc.desc: Valid object with styles as boolean → type mismatch.
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeValidObjectStylesBool_TypeMismatch)
{
    auto adapter = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":true})");
    ASSERT_NE(adapter, nullptr);
    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    ASSERT_EQ(result.validationIssues.size(), 1U);
    EXPECT_EQ(result.validationIssues[0].code, "ERROR_CODE_TYPE_MISMATCH");
}

/**
 * @tc.name: NormalizeValidObjectStylesArray_TypeMismatch
 * @tc.desc: Valid object with styles as array → type mismatch.
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeValidObjectStylesArray_TypeMismatch)
{
    auto adapter = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":[1,2,3]})");
    ASSERT_NE(adapter, nullptr);
    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    ASSERT_EQ(result.validationIssues.size(), 1U);
    EXPECT_EQ(result.validationIssues[0].code, "ERROR_CODE_TYPE_MISMATCH");
}

/**
 * @tc.name: NormalizeCompletedLog_WithComponentIdAndComponent
 * @tc.desc: Valid descriptor exercises the completion LOG at lines 60-64.
 *           The LOG format includes componentId and component strings.
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeCompletedLog_WithComponentIdAndComponent)
{
    auto adapter = JsonAdapter::Parse(R"({
        "id": "my-component-id",
        "component": "Row",
        "styles": {"width": 100}
    })");
    ASSERT_NE(adapter, nullptr);
    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    EXPECT_TRUE(result.styles.IsValid());
    EXPECT_TRUE(result.styles.IsObject());
    EXPECT_TRUE(result.validationIssues.empty());
    EXPECT_EQ(result.descriptor.GetString("id", ""), "my-component-id");
    EXPECT_EQ(result.descriptor.GetString("component", ""), "Row");
}

/**
 * @tc.name: NormalizeStylesNull_TypeMismatchReported
 * @tc.desc: Valid object with styles as null → styles.IsValid()=true, !IsObject()=true
 *           → type mismatch validation issue is reported.
 *           JSON null is a valid value but not an object, so the type mismatch check triggers.
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeStylesNull_TypeMismatchReported)
{
    auto adapter = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":null})");
    ASSERT_NE(adapter, nullptr);
    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    // styles is null → IsValid()=true but !IsObject() → type mismatch reported
    ASSERT_EQ(result.validationIssues.size(), 1U);
    EXPECT_EQ(result.validationIssues[0].code, "ERROR_CODE_TYPE_MISMATCH");
}

/**
 * @tc.name: NormalizeStartLog_InvalidDescriptor
 * @tc.desc: Invalid descriptor exercises the start LOG at lines 32-33.
 *           The LOG includes descriptor.IsValid() and descriptor.GetTypeName().
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeStartLog_InvalidDescriptor)
{
    JsonValue invalidValue;
    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(invalidValue);
    EXPECT_FALSE(result.IsValid());
}

/**
 * @tc.name: NormalizeStartLog_ValidDescriptor
 * @tc.desc: Valid descriptor exercises the start LOG with valid=true and type=object.
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeStartLog_ValidDescriptor)
{
    auto adapter = JsonAdapter::Parse(R"({"id":"test"})");
    ASSERT_NE(adapter, nullptr);
    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
}

/**
 * @tc.name: NormalizeValidObjectWithComplexStyles
 * @tc.desc: Complex styles object with nested values.
 *           Exercises full path: clone → IsObject → GetItem("styles") → IsObject → no issues → completion LOG.
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeValidObjectWithComplexStyles)
{
    auto adapter = JsonAdapter::Parse(R"({
        "id": "complex",
        "component": "Column",
        "styles": {
            "width": 100,
            "height": 200,
            "backgroundColor": "#FF000000",
            "padding": "10vp",
            "margin": {"top": 5, "right": 10}
        }
    })");
    ASSERT_NE(adapter, nullptr);
    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    EXPECT_TRUE(result.styles.IsValid());
    EXPECT_TRUE(result.styles.IsObject());
    EXPECT_TRUE(result.validationIssues.empty());
}

/**
 * @tc.name: NormalizeCompletedLog_NoStyles
 * @tc.desc: Valid descriptor without styles → completion LOG with hasStyles=false.
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeCompletedLog_NoStyles)
{
    auto adapter = JsonAdapter::Parse(R"({"id":"no-styles","component":"Text"})");
    ASSERT_NE(adapter, nullptr);
    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    EXPECT_FALSE(result.styles.IsValid());
    EXPECT_TRUE(result.validationIssues.empty());
}

/**
 * @tc.name: NormalizeCompletedLog_WithValidationIssues
 * @tc.desc: Valid descriptor with style type mismatch → completion LOG with validationIssues=1.
 *           Exercises the completion LOG format with non-zero validation issue count.
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeCompletedLog_WithValidationIssues)
{
    auto adapter = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":"wrong_type"})");
    ASSERT_NE(adapter, nullptr);
    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    ASSERT_EQ(result.validationIssues.size(), 1U);
}

/**
 * @tc.name: NormalizeEmptyObject_NoIdOrComponent
 * @tc.desc: Empty object → valid, no id/component → defaults in LOG.
 *           Exercises completion LOG with empty id and component strings.
 * @tc.type: FUNC
 */
TEST_F(ExtendedDescriptorNormalizerTddTest, NormalizeEmptyObject_NoIdOrComponent)
{
    auto adapter = JsonAdapter::Parse(R"({})");
    ASSERT_NE(adapter, nullptr);
    NormalizedExtendedDescriptor result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    EXPECT_EQ(result.descriptor.GetString("id", ""), "");
    EXPECT_EQ(result.descriptor.GetString("component", ""), "");
}

} // namespace
