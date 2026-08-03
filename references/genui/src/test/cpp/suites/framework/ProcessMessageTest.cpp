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

#include <string>
#include <vector>

#include "functions/RuntimeErrorDispatchBridge.h"
#include "functions/WarningDispatchBridge.h"
#include "utils/NapiUtils.h"

#include "NativeEntry.h"
#include "RenderManager.h"
#include "SurfaceErrorCodes.h"
#include "TestFixture.h"

using namespace NativeModule;

class ProcessMessageTest : public A2UITest {
protected:
    napi_env env_ = reinterpret_cast<napi_env>(0x100);
    napi_callback_info cbInfo_ = reinterpret_cast<napi_callback_info>(0x200);
    std::vector<int32_t> createdIds_;

    void SetUp() override
    {
        A2UITest::SetUp();
        WarningDispatchBridge::GetInstance().RegisterDispatchWarning(env_, CreateCallback("warningCallback"));
        RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    }

    napi_value CreateInt32Arg(int32_t value)
    {
        napi_value result = nullptr;
        mockNapiPtr_->CreateInt32(env_, value, &result);
        return result;
    }

    napi_value CreateCallback(const char* name = "runtimeErrorCallback")
    {
        napi_value callback = nullptr;
        mockNapiPtr_->CreateFunction(env_, name, NAPI_AUTO_LENGTH, nullptr, nullptr, &callback);
        return callback;
    }

    napi_value CreateStringArg(const std::string& value)
    {
        napi_value result = nullptr;
        mockNapiPtr_->CreateStringUtf8(env_, value.c_str(), NAPI_AUTO_LENGTH, &result);
        return result;
    }

    napi_value CreateCatalogArg()
    {
        napi_value result = nullptr;
        mockNapiPtr_->CreateObject(env_, &result);
        return result;
    }

    napi_value CreateCatalogArg(const std::string& catalogId)
    {
        napi_value result = CreateCatalogArg();
        mockNapiPtr_->SetNamedProperty(env_, result, "id", CreateStringArg(catalogId));
        return result;
    }

    void TrackAndCreateSlot(int32_t renderId)
    {
        RenderManager::GetInstance().CreateRenderSlot(renderId);
        createdIds_.push_back(renderId);
    }

    napi_value ProcessDsl(int32_t renderId, const std::string& dsl)
    {
        mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(renderId), CreateStringArg(dsl), CreateCatalogArg() });
        return ProcessMessage(env_, cbInfo_);
    }

    napi_value ProcessDsl(int32_t renderId, const std::string& dsl, napi_value catalogArg)
    {
        mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(renderId), CreateStringArg(dsl), catalogArg });
        return ProcessMessage(env_, cbInfo_);
    }

    bool GetBoolProperty(napi_value object, const char* name, bool fallback)
    {
        napi_value value = nullptr;
        if (mockNapiPtr_->GetNamedProperty(env_, object, name, &value) != napi_ok || value == nullptr) {
            return fallback;
        }

        bool result = fallback;
        mockNapiPtr_->GetValueBool(env_, value, &result);
        return result;
    }

    bool ExtractRuntimeErrorRequest(
        napi_value request, int32_t* renderId, int32_t* errorCode, std::string* errorMessage, std::string* source) const
    {
        auto objectIt = mockNapiPtr_->objectProperties_.find(request);
        if (objectIt == mockNapiPtr_->objectProperties_.end()) {
            return false;
        }

        const auto& properties = objectIt->second;
        auto renderIdIt = properties.find("renderId");
        auto errorCodeIt = properties.find("errorCode");
        auto errorMessageIt = properties.find("errorMessage");
        auto sourceIt = properties.find("source");
        if (renderIdIt == properties.end() || errorCodeIt == properties.end() || errorMessageIt == properties.end() ||
            sourceIt == properties.end()) {
            return false;
        }

        if (renderId != nullptr) {
            auto renderIdValue = mockNapiPtr_->numberValues_.find(renderIdIt->second);
            *renderId =
                renderIdValue != mockNapiPtr_->numberValues_.end() ? static_cast<int32_t>(renderIdValue->second) : 0;
        }
        if (errorCode != nullptr) {
            auto errorCodeValue = mockNapiPtr_->numberValues_.find(errorCodeIt->second);
            *errorCode =
                errorCodeValue != mockNapiPtr_->numberValues_.end() ? static_cast<int32_t>(errorCodeValue->second) : 0;
        }
        if (errorMessage != nullptr) {
            auto errorMessageValue = mockNapiPtr_->stringValues_.find(errorMessageIt->second);
            *errorMessage = errorMessageValue != mockNapiPtr_->stringValues_.end() ? errorMessageValue->second : "";
        }
        if (source != nullptr) {
            auto sourceValue = mockNapiPtr_->stringValues_.find(sourceIt->second);
            *source = sourceValue != mockNapiPtr_->stringValues_.end() ? sourceValue->second : "";
        }

        return true;
    }

    size_t RuntimeErrorDispatchCount() const
    {
        size_t count = 0;
        for (const auto& args : mockNapiPtr_->callFunctionArgsHistory_) {
            if (args.empty()) {
                continue;
            }
            if (ExtractRuntimeErrorRequest(args[0], nullptr, nullptr, nullptr, nullptr)) {
                ++count;
            }
        }
        return count;
    }

    bool ExtractWarningRequest(napi_value request, int32_t* renderId, std::string* code, std::string* message,
        std::string* path, std::string* itemType, std::string* itemName) const
    {
        auto objectIt = mockNapiPtr_->objectProperties_.find(request);
        if (objectIt == mockNapiPtr_->objectProperties_.end()) {
            return false;
        }

        const auto& properties = objectIt->second;
        auto renderIdIt = properties.find("renderId");
        auto codeIt = properties.find("code");
        auto messageIt = properties.find("message");
        auto pathIt = properties.find("path");
        auto itemTypeIt = properties.find("itemType");
        auto itemNameIt = properties.find("itemName");
        if (renderIdIt == properties.end() || codeIt == properties.end() || messageIt == properties.end() ||
            pathIt == properties.end() || itemTypeIt == properties.end() || itemNameIt == properties.end()) {
            return false;
        }

        if (renderId != nullptr) {
            auto renderIdValue = mockNapiPtr_->numberValues_.find(renderIdIt->second);
            *renderId =
                renderIdValue != mockNapiPtr_->numberValues_.end() ? static_cast<int32_t>(renderIdValue->second) : 0;
        }
        if (code != nullptr) {
            auto codeValue = mockNapiPtr_->stringValues_.find(codeIt->second);
            *code = codeValue != mockNapiPtr_->stringValues_.end() ? codeValue->second : "";
        }
        if (message != nullptr) {
            auto messageValue = mockNapiPtr_->stringValues_.find(messageIt->second);
            *message = messageValue != mockNapiPtr_->stringValues_.end() ? messageValue->second : "";
        }
        if (path != nullptr) {
            auto pathValue = mockNapiPtr_->stringValues_.find(pathIt->second);
            *path = pathValue != mockNapiPtr_->stringValues_.end() ? pathValue->second : "";
        }
        if (itemType != nullptr) {
            auto itemTypeValue = mockNapiPtr_->stringValues_.find(itemTypeIt->second);
            *itemType = itemTypeValue != mockNapiPtr_->stringValues_.end() ? itemTypeValue->second : "";
        }
        if (itemName != nullptr) {
            auto itemNameValue = mockNapiPtr_->stringValues_.find(itemNameIt->second);
            *itemName = itemNameValue != mockNapiPtr_->stringValues_.end() ? itemNameValue->second : "";
        }

        return true;
    }

    size_t WarningDispatchCount() const
    {
        size_t count = 0;
        for (const auto& args : mockNapiPtr_->callFunctionArgsHistory_) {
            if (args.empty()) {
                continue;
            }
            if (ExtractWarningRequest(args[0], nullptr, nullptr, nullptr, nullptr, nullptr, nullptr)) {
                ++count;
            }
        }
        return count;
    }

    bool FindLatestWarning(int32_t* renderId, std::string* code, std::string* message, std::string* path,
        std::string* itemType, std::string* itemName) const
    {
        for (auto iter = mockNapiPtr_->callFunctionArgsHistory_.rbegin();
             iter != mockNapiPtr_->callFunctionArgsHistory_.rend(); ++iter) {
            if (iter->empty()) {
                continue;
            }
            if (ExtractWarningRequest(iter->front(), renderId, code, message, path, itemType, itemName)) {
                return true;
            }
        }
        return false;
    }

    bool FindLatestRuntimeError(
        int32_t* renderId, int32_t* errorCode, std::string* errorMessage, std::string* source) const
    {
        for (auto iter = mockNapiPtr_->callFunctionArgsHistory_.rbegin();
             iter != mockNapiPtr_->callFunctionArgsHistory_.rend(); ++iter) {
            if (iter->empty()) {
                continue;
            }
            if (ExtractRuntimeErrorRequest(iter->front(), renderId, errorCode, errorMessage, source)) {
                return true;
            }
        }
        return false;
    }

    void TearDown() override
    {
        auto& renderManager = RenderManager::GetInstance();
        for (int32_t renderId : createdIds_) {
            if (renderManager.HasRenderSlot(renderId)) {
                renderManager.RemoveRenderSlot(renderId);
            }
        }
        createdIds_.clear();
        A2UITest::TearDown();
    }
};

TEST_F(ProcessMessageTest, L0_should_return_surface_already_exists_when_create_surface_id_duplicate)
{
    const int32_t renderId = 20260426;
    const std::string dsl = R"({"version":"v0.9","createSurface":{"surfaceId":"main","catalogId":"catalog"}})";
    TrackAndCreateSlot(renderId);

    napi_value firstResult = ProcessDsl(renderId, dsl);
    ASSERT_NE(firstResult, nullptr);
    EXPECT_TRUE(GetBoolProperty(firstResult, "success", false));

    napi_value secondResult = ProcessDsl(renderId, dsl);
    ASSERT_NE(secondResult, nullptr);
    EXPECT_FALSE(GetBoolProperty(secondResult, "success", true));
    EXPECT_EQ(NapiGetString(env_, secondResult, "errorCode", ""), "SURFACE_ALREADY_EXISTS");
    EXPECT_EQ(NapiGetString(env_, secondResult, "errorMessage", ""), "Surface already exists: main");
    EXPECT_EQ(NapiGetInt32(env_, secondResult, "surfaceResultCode", 0), SURFACE_RESULT_SURFACE_ALREADY_EXISTS);
}

TEST_F(ProcessMessageTest, L1_should_keep_processing_and_dispatch_runtime_error_when_dsl_length_exceeds_10kb)
{
    const int32_t renderId = 20260501;
    TrackAndCreateSlot(renderId);
    size_t beforeRuntimeErrorCount = RuntimeErrorDispatchCount();

    std::string padding(11 * 1024, 'X');
    std::string oversizedDsl =
        R"({"version":"v0.9","createSurface":{"surfaceId":"s","catalogId":"c","pad":")" + padding + "\"}}";

    napi_value result = ProcessDsl(renderId, oversizedDsl);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(GetBoolProperty(result, "success", false));
    EXPECT_EQ(RuntimeErrorDispatchCount(), beforeRuntimeErrorCount + 1);

    int32_t dispatchedRenderId = 0;
    int32_t dispatchedCode = 0;
    std::string dispatchedMessage;
    std::string dispatchedSource;
    ASSERT_TRUE(FindLatestRuntimeError(&dispatchedRenderId, &dispatchedCode, &dispatchedMessage, &dispatchedSource));
    EXPECT_EQ(dispatchedRenderId, renderId);
    EXPECT_EQ(dispatchedCode, SURFACE_ERROR_NATIVE_PROCESS_FAILED);
    EXPECT_NE(dispatchedMessage.find("exceeds maximum allowed"), std::string::npos);
    EXPECT_EQ(dispatchedSource, "NativeEntry::ProcessMessage");
}

TEST_F(ProcessMessageTest, L2_should_accept_dsl_when_length_exactly_10kb)
{
    const int32_t renderId = 20260502;
    TrackAndCreateSlot(renderId);

    std::string shortDsl = R"({"version":"v0.9","createSurface":{"surfaceId":"tiny","catalogId":"c"}})";
    ASSERT_LE(shortDsl.size(), static_cast<size_t>(10 * 1024));

    napi_value result = ProcessDsl(renderId, shortDsl);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(GetBoolProperty(result, "success", false));
}

TEST_F(ProcessMessageTest, L0_should_reject_create_surface_when_catalog_id_is_missing)
{
    const int32_t renderId = 20260503;
    TrackAndCreateSlot(renderId);

    napi_value result = ProcessDsl(renderId, R"({"version":"v0.9","createSurface":{"surfaceId":"missing-catalog"}})");
    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(GetBoolProperty(result, "success", true));
    EXPECT_EQ(NapiGetString(env_, result, "errorCode", ""), "CATALOG_ID_MISSING");
    EXPECT_EQ(NapiGetString(env_, result, "errorMessage", ""), "createSurface.catalogId is invalid");
    EXPECT_EQ(NapiGetInt32(env_, result, "surfaceResultCode", 0), SURFACE_RESULT_SCHEMA_CATALOG_ID_MISSING);
}

TEST_F(ProcessMessageTest,
    L0_should_dispatch_warning_and_keep_processing_when_create_surface_catalog_id_differs_from_controller_catalog)
{
    const int32_t renderId = 20260504;
    const std::string dsl = R"({"version":"v0.9","createSurface":{"surfaceId":"mismatch","catalogId":"dsl-catalog"}})";
    TrackAndCreateSlot(renderId);
    size_t beforeWarningCount = WarningDispatchCount();

    napi_value result = ProcessDsl(renderId, dsl, CreateCatalogArg("controller-catalog"));
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(GetBoolProperty(result, "success", false));
    EXPECT_EQ(WarningDispatchCount(), beforeWarningCount + 1);

    int32_t warningRenderId = 0;
    std::string warningCode;
    std::string warningMessage;
    std::string warningPath;
    std::string warningItemType;
    std::string warningItemName;
    ASSERT_TRUE(FindLatestWarning(
        &warningRenderId, &warningCode, &warningMessage, &warningPath, &warningItemType, &warningItemName));
    EXPECT_EQ(warningRenderId, renderId);
    EXPECT_EQ(warningCode, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(warningMessage, "Message catalogId differs from SurfaceController catalog.id and "
                              "native processing will use createSurface.catalogId");
    EXPECT_EQ(warningPath, "createSurface.catalogId");
    EXPECT_EQ(warningItemType, "message");
    EXPECT_EQ(warningItemName, "createSurface");
}

TEST_F(ProcessMessageTest, L0_should_reject_message_when_version_is_unsupported)
{
    const int32_t renderId = 20260505;
    const std::string dsl =
        R"({"version":"v1.0","createSurface":{"surfaceId":"unsupported-version","catalogId":"catalog"}})";
    TrackAndCreateSlot(renderId);
    size_t beforeWarningCount = WarningDispatchCount();

    napi_value result = ProcessDsl(renderId, dsl, CreateCatalogArg("catalog"));
    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(GetBoolProperty(result, "success", true));
    EXPECT_EQ(NapiGetString(env_, result, "errorCode", ""), "UNSUPPORTED_PROTOCOL_VERSION");
    EXPECT_EQ(NapiGetString(env_, result, "errorMessage", ""), "unsupported A2UI protocol version");
    EXPECT_EQ(NapiGetInt32(env_, result, "surfaceResultCode", 0), SURFACE_RESULT_UNSUPPORTED_PROTOCOL_VERSION);
    EXPECT_EQ(WarningDispatchCount(), beforeWarningCount + 1);

    int32_t warningRenderId = 0;
    std::string warningCode;
    std::string warningMessage;
    std::string warningPath;
    std::string warningItemType;
    std::string warningItemName;
    ASSERT_TRUE(FindLatestWarning(
        &warningRenderId, &warningCode, &warningMessage, &warningPath, &warningItemType, &warningItemName));
    EXPECT_EQ(warningRenderId, renderId);
    EXPECT_EQ(warningCode, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(warningMessage, "Message version is unsupported and native processing will reject this message");
    EXPECT_EQ(warningPath, "version");
    EXPECT_EQ(warningItemType, "message");
    EXPECT_EQ(warningItemName, "dsl");
}

TEST_F(ProcessMessageTest, L0_should_reject_message_when_version_is_missing_without_empty_dsl_error)
{
    const int32_t renderId = 20260506;
    const std::string dsl = R"({"createSurface":{"surfaceId":"missing-version","catalogId":"catalog"}})";
    TrackAndCreateSlot(renderId);
    size_t beforeWarningCount = WarningDispatchCount();

    napi_value result = ProcessDsl(renderId, dsl, CreateCatalogArg("catalog"));
    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(GetBoolProperty(result, "success", true));
    EXPECT_EQ(NapiGetString(env_, result, "errorCode", ""), "VERSION_INVALID");
    EXPECT_EQ(NapiGetString(env_, result, "errorMessage", ""), "version is invalid");
    EXPECT_EQ(NapiGetInt32(env_, result, "surfaceResultCode", 0), SURFACE_RESULT_SCHEMA_VERSION_INVALID);
    EXPECT_EQ(WarningDispatchCount(), beforeWarningCount + 1);

    int32_t warningRenderId = 0;
    std::string warningCode;
    std::string warningMessage;
    std::string warningPath;
    std::string warningItemType;
    std::string warningItemName;
    ASSERT_TRUE(FindLatestWarning(
        &warningRenderId, &warningCode, &warningMessage, &warningPath, &warningItemType, &warningItemName));
    EXPECT_EQ(warningRenderId, renderId);
    EXPECT_EQ(warningCode, "ERROR_CODE_REQUIRED_MISS");
    EXPECT_EQ(warningMessage, "Message version is required");
    EXPECT_EQ(warningPath, "version");
    EXPECT_EQ(warningItemType, "message");
    EXPECT_EQ(warningItemName, "dsl");
}

TEST_F(ProcessMessageTest, L0_should_reject_message_when_version_is_not_a_string)
{
    const int32_t renderId = 20260507;
    const std::string dsl = R"({"version":9,"createSurface":{"surfaceId":"number-version","catalogId":"catalog"}})";
    TrackAndCreateSlot(renderId);
    size_t beforeWarningCount = WarningDispatchCount();

    napi_value result = ProcessDsl(renderId, dsl, CreateCatalogArg("catalog"));
    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(GetBoolProperty(result, "success", true));
    EXPECT_EQ(NapiGetString(env_, result, "errorCode", ""), "VERSION_INVALID");
    EXPECT_EQ(NapiGetString(env_, result, "errorMessage", ""), "version is invalid");
    EXPECT_EQ(NapiGetInt32(env_, result, "surfaceResultCode", 0), SURFACE_RESULT_SCHEMA_VERSION_INVALID);
    EXPECT_EQ(WarningDispatchCount(), beforeWarningCount + 1);

    int32_t warningRenderId = 0;
    std::string warningCode;
    std::string warningMessage;
    std::string warningPath;
    std::string warningItemType;
    std::string warningItemName;
    ASSERT_TRUE(FindLatestWarning(
        &warningRenderId, &warningCode, &warningMessage, &warningPath, &warningItemType, &warningItemName));
    EXPECT_EQ(warningRenderId, renderId);
    EXPECT_EQ(warningCode, "ERROR_CODE_TYPE_MISMATCH");
    EXPECT_EQ(warningMessage, "Message version must be a string");
    EXPECT_EQ(warningPath, "version");
    EXPECT_EQ(warningItemType, "message");
    EXPECT_EQ(warningItemName, "dsl");
}

TEST_F(ProcessMessageTest, L0_should_reject_message_when_version_is_blank)
{
    const int32_t renderId = 20260508;
    const std::string dsl = R"({"version":"  ","createSurface":{"surfaceId":"blank-version","catalogId":"catalog"}})";
    TrackAndCreateSlot(renderId);
    size_t beforeWarningCount = WarningDispatchCount();

    napi_value result = ProcessDsl(renderId, dsl, CreateCatalogArg("catalog"));
    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(GetBoolProperty(result, "success", true));
    EXPECT_EQ(NapiGetString(env_, result, "errorCode", ""), "VERSION_INVALID");
    EXPECT_EQ(NapiGetString(env_, result, "errorMessage", ""), "version is invalid");
    EXPECT_EQ(NapiGetInt32(env_, result, "surfaceResultCode", 0), SURFACE_RESULT_SCHEMA_VERSION_INVALID);
    EXPECT_EQ(WarningDispatchCount(), beforeWarningCount + 1);

    int32_t warningRenderId = 0;
    std::string warningCode;
    std::string warningMessage;
    std::string warningPath;
    std::string warningItemType;
    std::string warningItemName;
    ASSERT_TRUE(FindLatestWarning(
        &warningRenderId, &warningCode, &warningMessage, &warningPath, &warningItemType, &warningItemName));
    EXPECT_EQ(warningRenderId, renderId);
    EXPECT_EQ(warningCode, "ERROR_CODE_REQUIRED_MISS");
    EXPECT_EQ(warningMessage, "Message version is required");
    EXPECT_EQ(warningPath, "version");
    EXPECT_EQ(warningItemType, "message");
    EXPECT_EQ(warningItemName, "dsl");
}

TEST_F(ProcessMessageTest, L0_should_dispatch_schema_warnings_for_parse_and_body_shape_errors)
{
    const int32_t renderId = 20260509;
    TrackAndCreateSlot(renderId);

    napi_value parseFailResult = ProcessDsl(renderId, "{", CreateCatalogArg("catalog"));
    ASSERT_NE(parseFailResult, nullptr);
    EXPECT_FALSE(GetBoolProperty(parseFailResult, "success", true));
    EXPECT_EQ(NapiGetString(env_, parseFailResult, "errorCode", ""), "JSON_PARSE_FAILED");

    int32_t warningRenderId = 0;
    std::string warningCode;
    std::string warningMessage;
    std::string warningPath;
    std::string warningItemType;
    std::string warningItemName;
    ASSERT_TRUE(FindLatestWarning(
        &warningRenderId, &warningCode, &warningMessage, &warningPath, &warningItemType, &warningItemName));
    EXPECT_EQ(warningRenderId, renderId);
    EXPECT_EQ(warningCode, "ERROR_CODE_SCHEMA_PARSE_FAILED");
    EXPECT_EQ(warningPath, "root");
    EXPECT_EQ(warningItemType, "message");
    EXPECT_EQ(warningItemName, "dsl");

    napi_value rootNotObjectResult = ProcessDsl(renderId, R"([])", CreateCatalogArg("catalog"));
    ASSERT_NE(rootNotObjectResult, nullptr);
    EXPECT_FALSE(GetBoolProperty(rootNotObjectResult, "success", true));
    EXPECT_EQ(NapiGetString(env_, rootNotObjectResult, "errorCode", ""), "ROOT_NOT_OBJECT");
    ASSERT_TRUE(FindLatestWarning(
        &warningRenderId, &warningCode, &warningMessage, &warningPath, &warningItemType, &warningItemName));
    EXPECT_EQ(warningCode, "ERROR_CODE_TYPE_MISMATCH");
    EXPECT_EQ(warningPath, "root");

    napi_value bodyNotObjectResult =
        ProcessDsl(renderId, R"({"version":"v0.9","createSurface":[]})", CreateCatalogArg("catalog"));
    ASSERT_NE(bodyNotObjectResult, nullptr);
    EXPECT_FALSE(GetBoolProperty(bodyNotObjectResult, "success", true));
    EXPECT_EQ(NapiGetString(env_, bodyNotObjectResult, "errorCode", ""), "MESSAGE_BODY_INVALID");
    ASSERT_TRUE(FindLatestWarning(
        &warningRenderId, &warningCode, &warningMessage, &warningPath, &warningItemType, &warningItemName));
    EXPECT_EQ(warningCode, "ERROR_CODE_TYPE_MISMATCH");
    EXPECT_EQ(warningPath, "createSurface");
    EXPECT_EQ(warningItemName, "createSurface");

    napi_value componentsInvalidResult =
        ProcessDsl(renderId, R"({"version":"v0.9","updateComponents":{"surfaceId":"components-bad","components":{}}})",
            CreateCatalogArg("catalog"));
    ASSERT_NE(componentsInvalidResult, nullptr);
    EXPECT_FALSE(GetBoolProperty(componentsInvalidResult, "success", true));
    EXPECT_EQ(NapiGetString(env_, componentsInvalidResult, "errorCode", ""), "COMPONENTS_INVALID");
    ASSERT_TRUE(FindLatestWarning(
        &warningRenderId, &warningCode, &warningMessage, &warningPath, &warningItemType, &warningItemName));
    EXPECT_EQ(warningCode, "ERROR_CODE_TYPE_MISMATCH");
    EXPECT_EQ(warningPath, "updateComponents.components");

    napi_value createSurfaceResult = ProcessDsl(renderId,
        R"({"version":"v0.9","createSurface":{"surfaceId":"update-data-model-target","catalogId":"catalog"}})",
        CreateCatalogArg("catalog"));
    ASSERT_NE(createSurfaceResult, nullptr);
    EXPECT_TRUE(GetBoolProperty(createSurfaceResult, "success", false));

    napi_value fallbackPathResult =
        ProcessDsl(renderId, R"({"version":"v0.9","updateDataModel":{"surfaceId":"update-data-model-target"}})",
            CreateCatalogArg("catalog"));
    ASSERT_NE(fallbackPathResult, nullptr);
    EXPECT_TRUE(GetBoolProperty(fallbackPathResult, "success", false));
    ASSERT_TRUE(FindLatestWarning(
        &warningRenderId, &warningCode, &warningMessage, &warningPath, &warningItemType, &warningItemName));
    EXPECT_EQ(warningCode, "ERROR_CODE_REQUIRED_MISS");
    EXPECT_EQ(warningPath, "updateDataModel.path");
}

TEST_F(ProcessMessageTest, L0_should_dispatch_schema_warnings_for_message_operation_and_root_field_errors)
{
    const int32_t renderId = 20260510;
    TrackAndCreateSlot(renderId);

    napi_value unknownOperationResult =
        ProcessDsl(renderId, R"({"version":"v0.9","unknownOp":{"surfaceId":"s"}})", CreateCatalogArg("catalog"));
    ASSERT_NE(unknownOperationResult, nullptr);
    EXPECT_FALSE(GetBoolProperty(unknownOperationResult, "success", true));
    EXPECT_EQ(NapiGetString(env_, unknownOperationResult, "errorCode", ""), "MESSAGE_OPERATION_INVALID");

    int32_t warningRenderId = 0;
    std::string warningCode;
    std::string warningMessage;
    std::string warningPath;
    std::string warningItemType;
    std::string warningItemName;
    ASSERT_TRUE(FindLatestWarning(
        &warningRenderId, &warningCode, &warningMessage, &warningPath, &warningItemType, &warningItemName));
    EXPECT_EQ(warningRenderId, renderId);
    EXPECT_EQ(warningCode, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(warningPath, "root");

    napi_value multipleBodiesResult = ProcessDsl(renderId,
        R"({"version":"v0.9","createSurface":{"surfaceId":"multi","catalogId":"catalog"},"deleteSurface":{"surfaceId":"multi"}})",
        CreateCatalogArg("catalog"));
    ASSERT_NE(multipleBodiesResult, nullptr);
    EXPECT_FALSE(GetBoolProperty(multipleBodiesResult, "success", true));
    EXPECT_EQ(NapiGetString(env_, multipleBodiesResult, "errorCode", ""), "MESSAGE_MULTIPLE_BODIES");
    ASSERT_TRUE(FindLatestWarning(
        &warningRenderId, &warningCode, &warningMessage, &warningPath, &warningItemType, &warningItemName));
    EXPECT_EQ(warningCode, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(warningPath, "root");

    napi_value missingSurfaceIdResult =
        ProcessDsl(renderId, R"({"version":"v0.9","deleteSurface":{}})", CreateCatalogArg("catalog"));
    ASSERT_NE(missingSurfaceIdResult, nullptr);
    EXPECT_FALSE(GetBoolProperty(missingSurfaceIdResult, "success", true));
    EXPECT_EQ(NapiGetString(env_, missingSurfaceIdResult, "errorCode", ""), "SURFACE_ID_MISSING");
    ASSERT_TRUE(FindLatestWarning(
        &warningRenderId, &warningCode, &warningMessage, &warningPath, &warningItemType, &warningItemName));
    EXPECT_EQ(warningCode, "ERROR_CODE_REQUIRED_MISS");
    EXPECT_EQ(warningPath, "deleteSurface.surfaceId");

    napi_value extraRootFieldResult = ProcessDsl(renderId,
        R"({"version":"v0.9","createSurface":{"surfaceId":"extra-root-field","catalogId":"catalog"},"extra":1})",
        CreateCatalogArg("catalog"));
    ASSERT_NE(extraRootFieldResult, nullptr);
    EXPECT_TRUE(GetBoolProperty(extraRootFieldResult, "success", false));
    ASSERT_TRUE(FindLatestWarning(
        &warningRenderId, &warningCode, &warningMessage, &warningPath, &warningItemType, &warningItemName));
    EXPECT_EQ(warningCode, "ERROR_CODE_UNDEFINED_FIELD");
    EXPECT_EQ(warningPath, "extra");
}
