#include <gtest/gtest.h>
#include <limits>
#include <list>
#include <memory>
#include <string>
#include <vector>

#define private public
#define protected public
#include "components/A2UI/A2UIComponent.h"
#include "components/extended/ExtendedGridComponent.h"
#include "components/extended/ExtendedGridTheme.h"
#include "components/extended/ExtendedListComponent.h"

#include "SurfaceSlot.h"
#include "mock_arkui_native_provider.h"
#undef protected
#undef private

#include "utils/JsonAdapter.h"

#include "A2UIArkUITypeConverter.h"
#include "ArkUINativeAPI.h"
#include "ArkUINodeApiAdapter.h"
#include "SchemaWarningTestHelper.h"
#include "TestFixture.h"

using namespace NativeModule;

namespace {

using SetAttributeCallback = int32_t (*)(ArkUI_NodeHandle, ArkUI_NodeAttributeType, ArkUI_AttributeItem*);
using CreateNodeCallback = ArkUI_NodeHandle (*)(ArkUI_NodeType);
using RegisterNodeEventCallback = int32_t (*)(ArkUI_NodeHandle, int32_t, int32_t, void*);

std::vector<ArkUI_NodeHandle> g_gridCreateNodeResults;
size_t g_gridCreateNodeIndex = 0;

class TypedParentComponent : public Component {
public:
    explicit TypedParentComponent(const std::string& type)
        : Component(reinterpret_cast<ArkUI_NodeHandle>(0x7100), false), type_(type)
    {}

    std::string GetType() const override
    {
        return type_;
    }

private:
    std::string type_;
};

class CountingNodeEventProvider : public MockArkUINativeProvider {
public:
    ArkUI_NodeHandle NodeEvent_GetNodeHandle(const ArkUI_NodeEvent* event) override
    {
        ++nodeEventGetNodeHandleCount;
        return MockArkUINativeProvider::NodeEvent_GetNodeHandle(event);
    }

    int32_t nodeEventGetNodeHandleCount = 0;
};

template<typename TComponent>
ArkUINodeApiAdapter CreateNodeApiAdapter(TComponent& component)
{
    return ArkUINodeApiAdapter([&component]() { return component.GetNativeView(); },
        [&component]() { return component.GetComponentId(); },
        [&component](
            float top, float right, float bottom, float left) { component.SetMargin(top, right, bottom, left); },
        [&component]() { component.ResetCommonMargin(); },
        [&component](const std::function<void()>& onClick) { component.RegisterOnClick(onClick); });
}

std::unique_ptr<JsonAdapter> ParseJson(const std::string& json)
{
    return JsonAdapter::Parse(json);
}

ChildListDescriptor BuildTemplateChildListDescriptor(
    const std::string& templateComponentId, const std::string& templatePath)
{
    ChildListDescriptor descriptor;
    descriptor.type = ChildListType::TEMPLATE_PATH;
    descriptor.templateComponentId = templateComponentId;
    descriptor.templatePath = templatePath;
    return descriptor;
}

const MockArkUINativeProvider::SetAttributeRecord* FindLastAttributeForNode(
    const MockArkUINativeProvider& provider, ArkUI_NodeHandle node, int32_t attribute)
{
    for (auto iter = provider.setAttributeRecords_.rbegin(); iter != provider.setAttributeRecords_.rend(); ++iter) {
        if (iter->nodeHandle == node && iter->attribute == attribute) {
            return &(*iter);
        }
    }
    return nullptr;
}

int32_t CountAttributeForNode(const MockArkUINativeProvider& provider, ArkUI_NodeHandle node, int32_t attribute)
{
    int32_t count = 0;
    for (const auto& record : provider.setAttributeRecords_) {
        if (record.nodeHandle == node && record.attribute == attribute) {
            ++count;
        }
    }
    return count;
}

void DispatchSizeChange(MockArkUINativeProvider& provider, ArkUI_NodeHandle node, float oldWidth, float newWidth)
{
    ArkUI_NodeEvent event {};
    ArkUI_NodeComponentEvent componentEvent {};
    componentEvent.data[0].f32 = oldWidth;
    componentEvent.data[2].f32 = newWidth;
    provider.SetNodeEventHandle(&event, node);
    provider.SetNodeEventType(&event, NODE_ON_SIZE_CHANGE);
    provider.SetNodeEventComponentEvent(&event, &componentEvent);
    EXPECT_TRUE(provider.DispatchNodeEvent(node, &event));
}

void DispatchAreaChange(MockArkUINativeProvider& provider, ArkUI_NodeHandle node, float oldWidth, float newWidth)
{
    ArkUI_NodeEvent event {};
    ArkUI_NodeComponentEvent componentEvent {};
    componentEvent.data[0].f32 = oldWidth;
    componentEvent.data[6].f32 = newWidth;
    provider.SetNodeEventHandle(&event, node);
    provider.SetNodeEventType(&event, NODE_EVENT_ON_AREA_CHANGE);
    provider.SetNodeEventComponentEvent(&event, &componentEvent);
    EXPECT_TRUE(provider.DispatchNodeEvent(node, &event));
}

int32_t ReturnSetAttributeFailure(ArkUI_NodeHandle node, ArkUI_NodeAttributeType attribute, ArkUI_AttributeItem* item)
{
    static_cast<void>(node);
    static_cast<void>(attribute);
    static_cast<void>(item);
    return -1;
}

ArkUI_NodeHandle CreateGridNodeFromSequence(ArkUI_NodeType type)
{
    static_cast<void>(type);
    if (g_gridCreateNodeIndex < g_gridCreateNodeResults.size()) {
        return g_gridCreateNodeResults[g_gridCreateNodeIndex++];
    }
    ++g_gridCreateNodeIndex;
    return reinterpret_cast<ArkUI_NodeHandle>(0x1);
}

int32_t ReturnRegisterNodeEventFailure(ArkUI_NodeHandle, int32_t, int32_t, void*)
{
    return -1;
}

class ScopedCreateNodeOverride {
public:
    ScopedCreateNodeOverride(ArkUI_NativeNodeAPI_1* nativeNodeApi, CreateNodeCallback callback)
        : nativeNodeApi_(nativeNodeApi),
          originalCreateNode_(nativeNodeApi == nullptr ? nullptr : nativeNodeApi->createNode)
    {
        if (nativeNodeApi_ != nullptr && callback != nullptr) {
            nativeNodeApi_->createNode = callback;
        }
    }

    ~ScopedCreateNodeOverride()
    {
        if (nativeNodeApi_ != nullptr) {
            nativeNodeApi_->createNode = originalCreateNode_;
        }
        g_gridCreateNodeResults.clear();
        g_gridCreateNodeIndex = 0;
    }

private:
    ArkUI_NativeNodeAPI_1* nativeNodeApi_ = nullptr;
    CreateNodeCallback originalCreateNode_ = nullptr;
};

class ScopedSetAttributeOverride {
public:
    ScopedSetAttributeOverride(ArkUI_NativeNodeAPI_1* nativeNodeApi, SetAttributeCallback callback)
        : nativeNodeApi_(nativeNodeApi),
          originalSetAttribute_(nativeNodeApi == nullptr ? nullptr : nativeNodeApi->setAttribute)
    {
        if (nativeNodeApi_ != nullptr && callback != nullptr) {
            nativeNodeApi_->setAttribute = callback;
        }
    }

    ~ScopedSetAttributeOverride()
    {
        if (nativeNodeApi_ != nullptr) {
            nativeNodeApi_->setAttribute = originalSetAttribute_;
        }
    }

private:
    ArkUI_NativeNodeAPI_1* nativeNodeApi_ = nullptr;
    SetAttributeCallback originalSetAttribute_ = nullptr;
};

class ScopedRegisterNodeEventOverride {
public:
    ScopedRegisterNodeEventOverride(ArkUI_NativeNodeAPI_1* nativeNodeApi, RegisterNodeEventCallback callback)
        : nativeNodeApi_(nativeNodeApi),
          originalCallback_(nativeNodeApi == nullptr ? nullptr : nativeNodeApi->registerNodeEvent)
    {
        if (nativeNodeApi_ != nullptr && callback != nullptr) {
            nativeNodeApi_->registerNodeEvent = callback;
        }
    }

    ~ScopedRegisterNodeEventOverride()
    {
        if (nativeNodeApi_ != nullptr) {
            nativeNodeApi_->registerNodeEvent = originalCallback_;
        }
    }

private:
    ArkUI_NativeNodeAPI_1* nativeNodeApi_ = nullptr;
    RegisterNodeEventCallback originalCallback_ = nullptr;
};

class ExtendedGridComponentTddTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        slot_.SetSurfaceId("surface-extended-grid");
        slot_.SetRenderId(1);
    }

    void PrepareWarningComponent(ExtendedGridComponent& component) const
    {
        component.SetRenderId(1);
        component.SetSurfaceId("surface-extended-grid");
        component.SetComponentId("gridRoot");
    }

    const MockArkUINativeProvider::SetAttributeRecord* FindLastSetAttribute(int32_t attribute) const
    {
        for (auto iter = mockArkUIPtr_->setAttributeRecords_.rbegin();
             iter != mockArkUIPtr_->setAttributeRecords_.rend(); ++iter) {
            if (iter->attribute == attribute) {
                return &(*iter);
            }
        }
        return nullptr;
    }

    bool HasResetAttribute(int32_t attribute) const
    {
        for (const auto& record : mockArkUIPtr_->resetAttributeRecords_) {
            if (record.attribute == attribute) {
                return true;
            }
        }
        return false;
    }

    SurfaceSlot slot_;
};

TEST_F(ExtendedGridComponentTddTest, should_apply_grid_defaults_and_style_fallbacks)
{
    std::unique_ptr<ExtendedGridComponent> ownedComponent = std::make_unique<ExtendedGridComponent>();
    EXPECT_EQ(ownedComponent->GetType(), "Grid");

    PropertyDeclaration declaration = ownedComponent->GetPrivatePropertyDeclaration("unknownProperty");
    EXPECT_TRUE(declaration.name.empty());
    EXPECT_FALSE(static_cast<bool>(declaration.applyValue));

    mockArkUIPtr_->setAttributeRecords_.clear();
    mockArkUIPtr_->resetAttributeRecords_.clear();
    std::unique_ptr<JsonAdapter> descriptor = JsonAdapter::CreateObject();
    ASSERT_NE(descriptor, nullptr);
    ownedComponent->ApplyPrivateAttributes(descriptor->GetRoot());

    const MockArkUINativeProvider::SetAttributeRecord* privateAlignItems =
        FindLastAttributeForNode(*mockArkUIPtr_, ownedComponent->GetNativeView(), NODE_GRID_ALIGN_ITEMS);
    const MockArkUINativeProvider::SetAttributeRecord* privateColumnsGap =
        FindLastAttributeForNode(*mockArkUIPtr_, ownedComponent->GetNativeView(), NODE_GRID_COLUMN_GAP);
    const MockArkUINativeProvider::SetAttributeRecord* privateRowsGap =
        FindLastAttributeForNode(*mockArkUIPtr_, ownedComponent->GetNativeView(), NODE_GRID_ROW_GAP);
    ASSERT_NE(privateAlignItems, nullptr);
    ASSERT_NE(privateColumnsGap, nullptr);
    ASSERT_NE(privateRowsGap, nullptr);
    ASSERT_FALSE(privateAlignItems->values.empty());
    ASSERT_FALSE(privateColumnsGap->values.empty());
    ASSERT_FALSE(privateRowsGap->values.empty());
    EXPECT_EQ(privateAlignItems->values[0].i32, 0);
    EXPECT_FLOAT_EQ(privateColumnsGap->values[0].f32, 0.0F);
    EXPECT_FLOAT_EQ(privateRowsGap->values[0].f32, 0.0F);
    EXPECT_FALSE(HasResetAttribute(NODE_GRID_ROW_TEMPLATE));

    ExtendedGridComponent invalidTemplateComponent;
    ArkUINodeApiAdapter invalidApplier = CreateNodeApiAdapter(invalidTemplateComponent);
    mockArkUIPtr_->setAttributeRecords_.clear();
    std::unique_ptr<JsonAdapter> invalidStyles = ParseJson(R"({
        "columnsTemplate": "",
        "rowsTemplate": 12345,
        "columnsGap": -12,
        "rowsGap": -6
    })");
    ASSERT_NE(invalidStyles, nullptr);
    invalidTemplateComponent.ApplyComponentSpecificStyles(invalidStyles->GetRoot(), invalidApplier);

    const MockArkUINativeProvider::SetAttributeRecord* columnsTemplate =
        FindLastAttributeForNode(*mockArkUIPtr_, invalidTemplateComponent.GetNativeView(), NODE_GRID_COLUMN_TEMPLATE);
    const MockArkUINativeProvider::SetAttributeRecord* rowsTemplate =
        FindLastAttributeForNode(*mockArkUIPtr_, invalidTemplateComponent.GetNativeView(), NODE_GRID_ROW_TEMPLATE);
    const MockArkUINativeProvider::SetAttributeRecord* columnsGap =
        FindLastAttributeForNode(*mockArkUIPtr_, invalidTemplateComponent.GetNativeView(), NODE_GRID_COLUMN_GAP);
    const MockArkUINativeProvider::SetAttributeRecord* rowsGap =
        FindLastAttributeForNode(*mockArkUIPtr_, invalidTemplateComponent.GetNativeView(), NODE_GRID_ROW_GAP);
    ASSERT_NE(columnsTemplate, nullptr);
    ASSERT_NE(rowsTemplate, nullptr);
    ASSERT_NE(columnsGap, nullptr);
    ASSERT_NE(rowsGap, nullptr);
    EXPECT_EQ(columnsTemplate->stringValue, "1fr");
    EXPECT_EQ(rowsTemplate->stringValue, "1fr");
    EXPECT_FLOAT_EQ(columnsGap->values[0].f32, 0.0F);
    EXPECT_FLOAT_EQ(rowsGap->values[0].f32, 0.0F);

    ExtendedGridComponent fullUpdateComponent;
    ArkUINodeApiAdapter fullUpdateApplier = CreateNodeApiAdapter(fullUpdateComponent);
    mockArkUIPtr_->setAttributeRecords_.clear();
    std::unique_ptr<JsonAdapter> emptyStyles = JsonAdapter::CreateObject();
    ASSERT_NE(emptyStyles, nullptr);
    fullUpdateComponent.ApplyComponentSpecificStyles(emptyStyles->GetRoot(), fullUpdateApplier);
    const MockArkUINativeProvider::SetAttributeRecord* defaultColumnsGap =
        FindLastAttributeForNode(*mockArkUIPtr_, fullUpdateComponent.GetNativeView(), NODE_GRID_COLUMN_GAP);
    const MockArkUINativeProvider::SetAttributeRecord* defaultRowsGap =
        FindLastAttributeForNode(*mockArkUIPtr_, fullUpdateComponent.GetNativeView(), NODE_GRID_ROW_GAP);
    ASSERT_NE(defaultColumnsGap, nullptr);
    ASSERT_NE(defaultRowsGap, nullptr);
    EXPECT_FLOAT_EQ(defaultColumnsGap->values[0].f32, 0.0F);
    EXPECT_FLOAT_EQ(defaultRowsGap->values[0].f32, 0.0F);
    EXPECT_FALSE(HasResetAttribute(NODE_GRID_ROW_TEMPLATE));

    ExtendedGridComponent deltaComponent;
    ArkUINodeApiAdapter deltaApplier = CreateNodeApiAdapter(deltaComponent);
    deltaComponent.isApplyingStyleDeltaUpdate_ = true;
    mockArkUIPtr_->setAttributeRecords_.clear();
    deltaComponent.ApplyComponentSpecificStyles(emptyStyles->GetRoot(), deltaApplier);
    EXPECT_EQ(FindLastAttributeForNode(*mockArkUIPtr_, deltaComponent.GetNativeView(), NODE_GRID_COLUMN_GAP), nullptr);
    EXPECT_EQ(FindLastAttributeForNode(*mockArkUIPtr_, deltaComponent.GetNativeView(), NODE_GRID_ROW_GAP), nullptr);

    ExtendedGridComponent nonObjectComponent;
    ArkUINodeApiAdapter nonObjectApplier = CreateNodeApiAdapter(nonObjectComponent);
    mockArkUIPtr_->setAttributeRecords_.clear();
    std::unique_ptr<JsonAdapter> nonObjectStyles = JsonAdapter::CreateNumber(2.0);
    ASSERT_NE(nonObjectStyles, nullptr);
    nonObjectComponent.ApplyComponentSpecificStyles(nonObjectStyles->GetRoot(), nonObjectApplier);
    EXPECT_EQ(FindLastAttributeForNode(*mockArkUIPtr_, nonObjectComponent.GetNativeView(), NODE_GRID_COLUMN_TEMPLATE),
        nullptr);
    EXPECT_EQ(
        FindLastAttributeForNode(*mockArkUIPtr_, nonObjectComponent.GetNativeView(), NODE_GRID_ROW_TEMPLATE), nullptr);

    ownedComponent.reset();
}

TEST_F(ExtendedGridComponentTddTest, should_apply_parent_flex_shrink_default_after_initial_invalid_value_is_attached)
{
    const std::vector<std::pair<std::string, float>> cases = { { "Column", 0.0F }, { "Row", 0.0F }, { "Flex", 1.0F } };
    for (const auto& [parentType, expectedValue] : cases) {
        SCOPED_TRACE(parentType);
        auto component = std::make_shared<ExtendedGridComponent>();
        auto descriptor = ParseJson(R"({"id":"grid","component":"Grid","styles":{"flexShrink":-1}})");
        ASSERT_NE(descriptor, nullptr);
        RenderContext context;
        context.renderId = 801;
        context.surfaceId = "surface-flex-shrink";
        ASSERT_TRUE(component->InitFromDescriptor(descriptor->GetRoot(), context));

        mockArkUIPtr_->setAttributeRecords_.clear();
        auto parent = std::make_shared<TypedParentComponent>(parentType);
        parent->SetComponentId("parent");
        parent->AddChild(component);

        const auto* record = FindLastAttributeForNode(*mockArkUIPtr_, component->GetNativeView(), NODE_FLEX_SHRINK);
        ASSERT_NE(record, nullptr);
        ASSERT_EQ(record->values.size(), 1U);
        EXPECT_FLOAT_EQ(record->values[0].f32, expectedValue);
    }
}

TEST_F(ExtendedGridComponentTddTest, should_not_override_valid_flex_shrink_when_attached)
{
    auto component = std::make_shared<ExtendedGridComponent>();
    auto descriptor = ParseJson(R"({"id":"grid","component":"Grid","styles":{"flexShrink":0.5}})");
    ASSERT_NE(descriptor, nullptr);
    RenderContext context;
    context.renderId = 802;
    context.surfaceId = "surface-flex-shrink";
    ASSERT_TRUE(component->InitFromDescriptor(descriptor->GetRoot(), context));

    mockArkUIPtr_->setAttributeRecords_.clear();
    auto parent = std::make_shared<TypedParentComponent>("Column");
    parent->SetComponentId("parent");
    parent->AddChild(component);

    EXPECT_EQ(FindLastAttributeForNode(*mockArkUIPtr_, component->GetNativeView(), NODE_FLEX_SHRINK), nullptr);
}

/**
 * @tc.name: Grid 非法模板单位回退
 * @tc.desc: columnsTemplate 使用不支持的 px 单位时回退为单列 1fr。
 * @tc.type: FUNC
 */
TEST_F(ExtendedGridComponentTddTest, should_fallback_unsupported_grid_template_units_to_single_fr)
{
    ExtendedGridComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);
    mockArkUIPtr_->setAttributeRecords_.clear();

    std::unique_ptr<JsonAdapter> styles = ParseJson(R"({
        "columnsTemplate": "1px 1px"
    })");
    ASSERT_NE(styles, nullptr);

    component.ApplyComponentSpecificStyles(styles->GetRoot(), applier);

    const MockArkUINativeProvider::SetAttributeRecord* columnsTemplate =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_COLUMN_TEMPLATE);
    ASSERT_NE(columnsTemplate, nullptr);
    EXPECT_EQ(columnsTemplate->stringValue, "1fr");
}

TEST_F(ExtendedGridComponentTddTest, should_validate_grid_style_schema_warnings)
{
    using namespace NativeModule::TestHelpers;

    RegisterWarningDispatchCallback(mockNapiPtr_);

    ExtendedGridComponent component;
    PrepareWarningComponent(component);

    std::unique_ptr<JsonAdapter> invalidStaticStyles = ParseJson(R"({
        "columnsTemplate": "",
        "rowsTemplate": 1,
        "columnsGap": "bad",
        "rowsGap": -1
    })");
    ASSERT_NE(invalidStaticStyles, nullptr);
    component.ValidateComponentSpecificStylesSchema(invalidStaticStyles->GetRoot());
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.columnsTemplate"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.rowsTemplate"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.columnsGap"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.rowsGap"), 1U);

    mockNapiPtr_->callFunctionArgsHistory_.clear();
    std::unique_ptr<JsonAdapter> unsupportedUnitStyles = ParseJson(R"({
        "columnsTemplate": "1px 1px"
    })");
    ASSERT_NE(unsupportedUnitStyles, nullptr);
    component.ValidateComponentSpecificStylesSchema(unsupportedUnitStyles->GetRoot());
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.columnsTemplate"), 1U);

    mockNapiPtr_->callFunctionArgsHistory_.clear();
    std::unique_ptr<JsonAdapter> responsiveTypeMismatch = ParseJson(R"({
        "columnsTemplate": { "xs": 1 }
    })");
    ASSERT_NE(responsiveTypeMismatch, nullptr);
    component.ValidateComponentSpecificStylesSchema(responsiveTypeMismatch->GetRoot());
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.columnsTemplate.xs"), 1U);

    mockNapiPtr_->callFunctionArgsHistory_.clear();
    std::unique_ptr<JsonAdapter> responsiveInvalidValue = ParseJson(R"({
        "rowsTemplate": { "sm": "" }
    })");
    ASSERT_NE(responsiveInvalidValue, nullptr);
    component.ValidateComponentSpecificStylesSchema(responsiveInvalidValue->GetRoot());
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.rowsTemplate.sm"), 1U);

    mockNapiPtr_->callFunctionArgsHistory_.clear();
    std::unique_ptr<JsonAdapter> emptyResponsiveObject = ParseJson(R"({
        "columnsTemplate": {}
    })");
    ASSERT_NE(emptyResponsiveObject, nullptr);
    component.ValidateComponentSpecificStylesSchema(emptyResponsiveObject->GetRoot());
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.columnsTemplate"), 1U);

    mockNapiPtr_->callFunctionArgsHistory_.clear();
    std::unique_ptr<JsonAdapter> dynamicStyles = ParseJson(R"({
        "columnsTemplate": { "path": "item.columns" }
    })");
    ASSERT_NE(dynamicStyles, nullptr);
    component.ValidateComponentSpecificStylesSchema(dynamicStyles->GetRoot());
    EXPECT_TRUE(mockNapiPtr_->callFunctionArgsHistory_.empty());
}

TEST_F(ExtendedGridComponentTddTest, should_parse_template_configs_and_resolve_breakpoints)
{
    ExtendedGridComponent::GridTemplateConfig config;

    std::unique_ptr<JsonAdapter> emptyString = JsonAdapter::CreateString("");
    ASSERT_NE(emptyString, nullptr);
    ASSERT_TRUE(ExtendedGridComponent::ParseTemplateConfig(emptyString->GetRoot(), config));
    EXPECT_EQ(config.mode, ExtendedGridComponent::TemplateMode::FIXED);
    EXPECT_EQ(config.fixedValue, "1fr");

    std::unique_ptr<JsonAdapter> fixedString = JsonAdapter::CreateString("1fr 2fr");
    ASSERT_NE(fixedString, nullptr);
    ASSERT_TRUE(ExtendedGridComponent::ParseTemplateConfig(fixedString->GetRoot(), config));
    EXPECT_EQ(config.mode, ExtendedGridComponent::TemplateMode::FIXED);
    EXPECT_EQ(config.fixedValue, "1fr 2fr");

    std::unique_ptr<JsonAdapter> nonObject = JsonAdapter::CreateNumber(5.0);
    ASSERT_NE(nonObject, nullptr);
    EXPECT_FALSE(ExtendedGridComponent::ParseTemplateConfig(nonObject->GetRoot(), config));

    std::unique_ptr<JsonAdapter> invalidResponsiveType = ParseJson(R"({ "xs": 1 })");
    ASSERT_NE(invalidResponsiveType, nullptr);
    EXPECT_FALSE(ExtendedGridComponent::ParseTemplateConfig(invalidResponsiveType->GetRoot(), config));

    std::unique_ptr<JsonAdapter> emptyResponsiveValue = ParseJson(R"({ "xs": "" })");
    ASSERT_NE(emptyResponsiveValue, nullptr);
    EXPECT_FALSE(ExtendedGridComponent::ParseTemplateConfig(emptyResponsiveValue->GetRoot(), config));

    std::unique_ptr<JsonAdapter> responsive = ParseJson(R"({
        "md": "2fr 1fr",
        "xl": "4fr 2fr"
    })");
    ASSERT_NE(responsive, nullptr);
    ASSERT_TRUE(ExtendedGridComponent::ParseTemplateConfig(responsive->GetRoot(), config));
    EXPECT_EQ(config.mode, ExtendedGridComponent::TemplateMode::RESPONSIVE);

    ThemeContext xsContext;
    xsContext.breakpoint = Breakpoint::XS;
    EXPECT_EQ(ExtendedGridComponent::ResolveResponsiveTemplate(config, xsContext), "2fr 1fr");

    ThemeContext smContext;
    smContext.breakpoint = Breakpoint::SM;
    EXPECT_EQ(ExtendedGridComponent::ResolveResponsiveTemplate(config, smContext), "2fr 1fr");

    ThemeContext mdContext;
    mdContext.breakpoint = Breakpoint::MD;
    EXPECT_EQ(ExtendedGridComponent::ResolveResponsiveTemplate(config, mdContext), "2fr 1fr");

    ThemeContext lgContext;
    lgContext.breakpoint = Breakpoint::LG;
    EXPECT_EQ(ExtendedGridComponent::ResolveResponsiveTemplate(config, lgContext), "2fr 1fr");

    ThemeContext xlContext;
    xlContext.breakpoint = Breakpoint::XL;
    EXPECT_EQ(ExtendedGridComponent::ResolveResponsiveTemplate(config, xlContext), "4fr 2fr");

    ThemeContext invalidContext;
    invalidContext.breakpoint = static_cast<Breakpoint>(99);
    EXPECT_EQ(ExtendedGridComponent::ResolveResponsiveTemplate(config, invalidContext), "2fr 1fr");

    ExtendedGridComponent component;
    ThemeContext defaultContext = component.ResolveThemeContext();
    EXPECT_EQ(defaultContext.breakpoint, ThemeContext().breakpoint);
}

TEST_F(ExtendedGridComponentTddTest, should_apply_template_modes_and_skip_missing_native_view_paths)
{
    ExtendedGridComponent component;
    ThemeContext context;
    context.breakpoint = Breakpoint::XL;

    component.columnsTemplateConfig_.mode = ExtendedGridComponent::TemplateMode::RESPONSIVE;
    component.columnsTemplateConfig_.responsiveValues.fill("");
    component.columnsTemplateConfig_.responsiveValues[3] = "3fr 1fr";
    component.columnsTemplateConfig_.responsiveValues[4] = "5fr 2fr";
    component.rowsTemplateConfig_.mode = ExtendedGridComponent::TemplateMode::RESPONSIVE;
    component.rowsTemplateConfig_.responsiveValues.fill("");
    component.rowsTemplateConfig_.responsiveValues[2] = "40 80";

    mockArkUIPtr_->setAttributeRecords_.clear();
    component.OnConfigChange(context);

    const MockArkUINativeProvider::SetAttributeRecord* responsiveColumns =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_COLUMN_TEMPLATE);
    const MockArkUINativeProvider::SetAttributeRecord* responsiveRows =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_ROW_TEMPLATE);
    ASSERT_NE(responsiveColumns, nullptr);
    ASSERT_NE(responsiveRows, nullptr);
    EXPECT_EQ(responsiveColumns->stringValue, "5fr 2fr");
    EXPECT_EQ(responsiveRows->stringValue, "40 80");

    component.columnsTemplateConfig_.mode = ExtendedGridComponent::TemplateMode::RESET;
    component.rowsTemplateConfig_.mode = ExtendedGridComponent::TemplateMode::THEME_DEFAULT;
    mockArkUIPtr_->setAttributeRecords_.clear();
    mockArkUIPtr_->resetAttributeRecords_.clear();
    component.OnConfigChange(context);

    ExtendedGridTheme theme(context);
    const MockArkUINativeProvider::SetAttributeRecord* defaultColumns =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_COLUMN_TEMPLATE);
    const MockArkUINativeProvider::SetAttributeRecord* defaultRows =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_ROW_TEMPLATE);
    ASSERT_NE(defaultColumns, nullptr);
    EXPECT_EQ(defaultColumns->stringValue, theme.GetColumnsTemplate());
    EXPECT_EQ(defaultRows, nullptr);
    EXPECT_FALSE(HasResetAttribute(NODE_GRID_ROW_TEMPLATE));

    ArkUI_NodeHandle originalView = component.nativeView_;
    component.nativeView_ = nullptr;
    mockArkUIPtr_->setAttributeRecords_.clear();
    component.ApplyRowsTemplateForContext(context);
    component.SetColumnsTemplate("ignored");
    component.SetRowsTemplate("ignored");
    component.SetColumnsGap(8.0F);
    component.SetRowsGap(10.0F);
    component.DetachGridItemNode(nullptr);
    EXPECT_TRUE(mockArkUIPtr_->setAttributeRecords_.empty());
    component.nativeView_ = originalView;
}

TEST_F(ExtendedGridComponentTddTest, should_apply_templates_from_component_breakpoint)
{
    ExtendedGridComponent component;
    component.renderContext_.apiVersion = MIN_API_VERSION_SIZE_CHANGE;
    component.RegisterComponentSpecificListeners();
    EXPECT_EQ(mockArkUIPtr_->registeredNodeEvents_[component.GetNativeView()].count(NODE_ON_SIZE_CHANGE), 1U);

    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);
    std::unique_ptr<JsonAdapter> styles = ParseJson(R"({
        "columnsTemplate": { "xs": "1fr", "md": "1fr 1fr" },
        "rowsTemplate": { "xs": "2fr", "md": "2fr 2fr" }
    })");
    ASSERT_NE(styles, nullptr);
    component.ApplyComponentSpecificStyles(styles->GetRoot(), applier);

    ThemeContext globalContext;
    globalContext.breakpoint = Breakpoint::XL;
    component.OnConfigChange(globalContext);
    const MockArkUINativeProvider::SetAttributeRecord* globalColumns =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_COLUMN_TEMPLATE);
    const MockArkUINativeProvider::SetAttributeRecord* globalRows =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_ROW_TEMPLATE);
    ASSERT_NE(globalColumns, nullptr);
    ASSERT_NE(globalRows, nullptr);
    EXPECT_EQ(globalColumns->stringValue, "1fr 1fr");
    EXPECT_EQ(globalRows->stringValue, "2fr 2fr");

    mockArkUIPtr_->setAttributeRecords_.clear();
    DispatchSizeChange(*mockArkUIPtr_, component.GetNativeView(), 0.0F, 360.0F);
    const MockArkUINativeProvider::SetAttributeRecord* componentColumns =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_COLUMN_TEMPLATE);
    const MockArkUINativeProvider::SetAttributeRecord* componentRows =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_ROW_TEMPLATE);
    ASSERT_NE(componentColumns, nullptr);
    ASSERT_NE(componentRows, nullptr);
    EXPECT_EQ(componentColumns->stringValue, "1fr");
    EXPECT_EQ(componentRows->stringValue, "2fr");
    ASSERT_TRUE(component.componentBreakpoint_.has_value());
    EXPECT_EQ(component.componentBreakpoint_.value(), Breakpoint::SM);
}

TEST_F(ExtendedGridComponentTddTest, should_use_area_change_below_api_21_and_ignore_position_only_updates)
{
    {
        ExtendedListComponent list;
        list.renderContext_.apiVersion = 20;
        list.RegisterComponentSpecificListeners();
        EXPECT_EQ(mockArkUIPtr_->registeredNodeEvents_[list.GetNativeView()].count(NODE_EVENT_ON_AREA_CHANGE), 1U);
        EXPECT_EQ(mockArkUIPtr_->registeredNodeEvents_[list.GetNativeView()].count(NODE_ON_SIZE_CHANGE), 0U);

        DispatchAreaChange(*mockArkUIPtr_, list.GetNativeView(), 360.0F, 360.0F);
        EXPECT_FALSE(list.componentBreakpoint_.has_value());
        DispatchAreaChange(*mockArkUIPtr_, list.GetNativeView(), 360.0F, 640.0F);
        ASSERT_TRUE(list.componentBreakpoint_.has_value());
        EXPECT_EQ(list.componentBreakpoint_.value(), Breakpoint::MD);
    }

    {
        ExtendedGridComponent grid;
        grid.renderContext_.apiVersion = 20;
        grid.RegisterComponentSpecificListeners();
        EXPECT_EQ(mockArkUIPtr_->registeredNodeEvents_[grid.GetNativeView()].count(NODE_EVENT_ON_AREA_CHANGE), 1U);
        EXPECT_EQ(mockArkUIPtr_->registeredNodeEvents_[grid.GetNativeView()].count(NODE_ON_SIZE_CHANGE), 0U);

        DispatchAreaChange(*mockArkUIPtr_, grid.GetNativeView(), 360.0F, 360.0F);
        EXPECT_FALSE(grid.componentBreakpoint_.has_value());
        DispatchAreaChange(*mockArkUIPtr_, grid.GetNativeView(), 360.0F, 640.0F);
        ASSERT_TRUE(grid.componentBreakpoint_.has_value());
        EXPECT_EQ(grid.componentBreakpoint_.value(), Breakpoint::MD);
    }
}

TEST_F(ExtendedGridComponentTddTest, should_keep_size_change_at_api_21)
{
    {
        ExtendedListComponent list;
        list.renderContext_.apiVersion = 21;
        list.RegisterComponentSpecificListeners();
        EXPECT_EQ(mockArkUIPtr_->registeredNodeEvents_[list.GetNativeView()].count(NODE_ON_SIZE_CHANGE), 1U);
        EXPECT_EQ(mockArkUIPtr_->registeredNodeEvents_[list.GetNativeView()].count(NODE_EVENT_ON_AREA_CHANGE), 0U);
    }

    {
        ExtendedGridComponent grid;
        grid.renderContext_.apiVersion = 21;
        grid.RegisterComponentSpecificListeners();
        EXPECT_EQ(mockArkUIPtr_->registeredNodeEvents_[grid.GetNativeView()].count(NODE_ON_SIZE_CHANGE), 1U);
        EXPECT_EQ(mockArkUIPtr_->registeredNodeEvents_[grid.GetNativeView()].count(NODE_EVENT_ON_AREA_CHANGE), 0U);
    }
}

TEST_F(ExtendedGridComponentTddTest, should_dispatch_payload_event_and_cleanup_registration)
{
    ArkUI_NodeHandle handle = reinterpret_cast<ArkUI_NodeHandle>(0x9701);
    A2UINodeEvent* receivedEvent = nullptr;
    {
        auto component = std::make_shared<A2UIComponent>(handle, false);
        ASSERT_TRUE(component->RegisterNodeEventHandlerWithEvent(
            A2UINodeEventType::ON_SIZE_CHANGE, [&receivedEvent](A2UINodeEvent* event) { receivedEvent = event; }));
        EXPECT_EQ(mockArkUIPtr_->registeredNodeEvents_[handle].count(NODE_ON_SIZE_CHANGE), 1U);

        ArkUI_NodeEvent event {};
        mockArkUIPtr_->SetNodeEventHandle(&event, handle);
        mockArkUIPtr_->SetNodeEventType(&event, NODE_ON_SIZE_CHANGE);
        EXPECT_TRUE(mockArkUIPtr_->DispatchNodeEvent(handle, &event));
        EXPECT_EQ(receivedEvent, &event);
    }
    EXPECT_EQ(mockArkUIPtr_->registeredNodeEvents_.count(handle), 0U);
}

TEST_F(ExtendedGridComponentTddTest, should_ignore_null_payload_event_before_querying_node_handle)
{
    auto provider = std::make_unique<CountingNodeEventProvider>();
    CountingNodeEventProvider* providerPtr = provider.get();
    mockArkUIPtr_ = providerPtr;
    ArkUINativeAPI::SetProvider(std::move(provider));

    A2UIComponent::NodeEventReceiver(nullptr);

    EXPECT_EQ(providerPtr->nodeEventGetNodeHandleCount, 0);
}

TEST_F(ExtendedGridComponentTddTest, should_reject_payload_handler_when_registration_fails)
{
    ArkUI_NativeNodeAPI_1* nativeNodeApi = ArkUINativeAPI::GetInstance().GetNativeNodeAPI();
    ASSERT_NE(nativeNodeApi, nullptr);
    ScopedRegisterNodeEventOverride override(nativeNodeApi, ReturnRegisterNodeEventFailure);

    A2UIComponent component(reinterpret_cast<ArkUI_NodeHandle>(0x9702), false);
    EXPECT_FALSE(component.RegisterNodeEventHandlerWithEvent(A2UINodeEventType::ON_SIZE_CHANGE, [](A2UINodeEvent*) {}));
    EXPECT_TRUE(component.nodeEventHandlers_.empty());
}

TEST_F(ExtendedGridComponentTddTest, should_apply_and_deduplicate_list_breakpoint_lanes)
{
    ExtendedListComponent component;
    component.renderContext_.apiVersion = MIN_API_VERSION_SIZE_CHANGE;
    component.RegisterComponentSpecificListeners();
    EXPECT_EQ(mockArkUIPtr_->registeredNodeEvents_[component.GetNativeView()].count(NODE_ON_SIZE_CHANGE), 1U);

    ThemeContext globalContext;
    globalContext.breakpoint = Breakpoint::MD;
    component.OnConfigChange(globalContext);
    const MockArkUINativeProvider::SetAttributeRecord* globalLanes =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_LIST_LANES);
    ASSERT_NE(globalLanes, nullptr);
    ASSERT_FALSE(globalLanes->values.empty());
    EXPECT_EQ(globalLanes->values[0].i32, 2);

    mockArkUIPtr_->setAttributeRecords_.clear();
    DispatchSizeChange(*mockArkUIPtr_, component.GetNativeView(), 0.0F, 360.0F);
    const MockArkUINativeProvider::SetAttributeRecord* componentLanes =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_LIST_LANES);
    ASSERT_NE(componentLanes, nullptr);
    ASSERT_FALSE(componentLanes->values.empty());
    EXPECT_EQ(componentLanes->values[0].i32, 1);

    mockArkUIPtr_->setAttributeRecords_.clear();
    DispatchSizeChange(*mockArkUIPtr_, component.GetNativeView(), 360.0F, 400.0F);
    EXPECT_EQ(CountAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_LIST_LANES), 0);
    DispatchSizeChange(*mockArkUIPtr_, component.GetNativeView(), 400.0F, 640.0F);
    EXPECT_EQ(CountAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_LIST_LANES), 1);
}

TEST_F(ExtendedGridComponentTddTest, should_filter_invalid_list_breakpoint_widths_and_track_crossings)
{
    ExtendedListComponent component;
    component.renderContext_.apiVersion = MIN_API_VERSION_SIZE_CHANGE;
    component.RegisterComponentSpecificListeners();
    mockArkUIPtr_->setAttributeRecords_.clear();

    DispatchSizeChange(*mockArkUIPtr_, component.GetNativeView(), 0.0F, 300.0F);
    DispatchSizeChange(*mockArkUIPtr_, component.GetNativeView(), 300.0F, 360.0F);
    DispatchSizeChange(*mockArkUIPtr_, component.GetNativeView(), 360.0F, 640.0F);
    EXPECT_EQ(CountAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_LIST_LANES), 3);

    DispatchSizeChange(*mockArkUIPtr_, component.GetNativeView(), 640.0F, 640.0F);
    DispatchSizeChange(*mockArkUIPtr_, component.GetNativeView(), 640.0F, 0.0F);
    DispatchSizeChange(*mockArkUIPtr_, component.GetNativeView(), 640.0F, std::numeric_limits<float>::quiet_NaN());
    DispatchSizeChange(*mockArkUIPtr_, component.GetNativeView(), 640.0F, std::numeric_limits<float>::infinity());
    component.HandleSizeChange(nullptr);
    ArkUI_NodeEvent emptyEvent {};
    mockArkUIPtr_->SetNodeEventHandle(&emptyEvent, component.GetNativeView());
    mockArkUIPtr_->SetNodeEventType(&emptyEvent, NODE_ON_SIZE_CHANGE);
    EXPECT_TRUE(mockArkUIPtr_->DispatchNodeEvent(component.GetNativeView(), &emptyEvent));
    EXPECT_EQ(CountAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_LIST_LANES), 3);
}

TEST_F(ExtendedGridComponentTddTest, should_deduplicate_component_breakpoint_updates_and_filter_invalid_widths)
{
    ExtendedGridComponent component;
    component.renderContext_.apiVersion = MIN_API_VERSION_SIZE_CHANGE;
    component.RegisterComponentSpecificListeners();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);
    std::unique_ptr<JsonAdapter> styles = ParseJson(R"({
        "columnsTemplate": { "xs": "xs", "sm": "sm", "md": "md" },
        "rowsTemplate": { "xs": "1fr", "sm": "2fr", "md": "3fr" }
    })");
    ASSERT_NE(styles, nullptr);
    component.ApplyComponentSpecificStyles(styles->GetRoot(), applier);

    DispatchSizeChange(*mockArkUIPtr_, component.GetNativeView(), 0.0F, 500.0F);
    mockArkUIPtr_->setAttributeRecords_.clear();

    DispatchSizeChange(*mockArkUIPtr_, component.GetNativeView(), 500.0F, 800.0F);
    DispatchSizeChange(*mockArkUIPtr_, component.GetNativeView(), 800.0F, 820.0F);
    DispatchSizeChange(*mockArkUIPtr_, component.GetNativeView(), 820.0F, 820.0F);
    DispatchSizeChange(*mockArkUIPtr_, component.GetNativeView(), 820.0F, 0.0F);
    DispatchSizeChange(*mockArkUIPtr_, component.GetNativeView(), 820.0F, std::numeric_limits<float>::quiet_NaN());
    DispatchSizeChange(*mockArkUIPtr_, component.GetNativeView(), 820.0F, std::numeric_limits<float>::infinity());
    component.HandleSizeChange(nullptr);
    ArkUI_NodeEvent emptyEvent {};
    mockArkUIPtr_->SetNodeEventHandle(&emptyEvent, component.GetNativeView());
    mockArkUIPtr_->SetNodeEventType(&emptyEvent, NODE_ON_SIZE_CHANGE);
    EXPECT_TRUE(mockArkUIPtr_->DispatchNodeEvent(component.GetNativeView(), &emptyEvent));
    EXPECT_EQ(CountAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_COLUMN_TEMPLATE), 1);
    EXPECT_EQ(CountAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_ROW_TEMPLATE), 1);

    DispatchSizeChange(*mockArkUIPtr_, component.GetNativeView(), 820.0F, 300.0F);
    DispatchSizeChange(*mockArkUIPtr_, component.GetNativeView(), 300.0F, 360.0F);
    DispatchSizeChange(*mockArkUIPtr_, component.GetNativeView(), 360.0F, 640.0F);
    EXPECT_EQ(CountAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_COLUMN_TEMPLATE), 4);
    EXPECT_EQ(CountAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_ROW_TEMPLATE), 4);
    const MockArkUINativeProvider::SetAttributeRecord* finalRows =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_ROW_TEMPLATE);
    ASSERT_NE(finalRows, nullptr);
    EXPECT_EQ(finalRows->stringValue, "3fr");
    ASSERT_TRUE(component.componentBreakpoint_.has_value());
    EXPECT_EQ(component.componentBreakpoint_.value(), Breakpoint::MD);
}

TEST_F(ExtendedGridComponentTddTest, should_cover_child_slot_management_branches)
{
    auto childA = std::make_shared<A2UIComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x9401), false);
    auto childB = std::make_shared<A2UIComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x9402), false);

    ExtendedGridComponent lazyComponent;
    lazyComponent.mode_ = ExtendedGridComponent::Mode::LAZY;
    lazyComponent.OnAddChild(childA, 0);
    EXPECT_TRUE(lazyComponent.gridItems_.empty());

    ExtendedGridComponent noViewComponent;
    ArkUI_NodeHandle originalView = noViewComponent.nativeView_;
    noViewComponent.nativeView_ = nullptr;
    noViewComponent.OnAddChild(childA, 0);
    EXPECT_TRUE(noViewComponent.gridItems_.empty());
    noViewComponent.nativeView_ = originalView;

    ExtendedGridComponent component;
    ArkUI_NativeNodeAPI_1* nativeNodeApi = ArkUINativeAPI::GetInstance().GetNativeNodeAPI();
    ASSERT_NE(nativeNodeApi, nullptr);
    g_gridCreateNodeResults = { reinterpret_cast<ArkUI_NodeHandle>(0x9501),
        reinterpret_cast<ArkUI_NodeHandle>(0x9502) };
    ScopedCreateNodeOverride createNodeOverride(nativeNodeApi, CreateGridNodeFromSequence);

    component.OnAddChild(nullptr, 0);
    component.OnMoveChild(nullptr, 0, 0);
    component.OnMoveChild(childA, 0, 0);
    component.OnRemoveChild(nullptr);
    component.OnRemoveChild(childA);

    component.OnAddChild(childA, 0);
    component.OnAddChild(childB, 1);
    ASSERT_EQ(component.gridItems_.size(), 2U);
    EXPECT_EQ(component.gridItems_[0].child.lock(), childA);
    EXPECT_EQ(component.gridItems_[1].child.lock(), childB);
    EXPECT_EQ(component.gridItems_[0].itemNode, reinterpret_cast<ArkUI_NodeHandle>(0x9501));
    EXPECT_EQ(component.gridItems_[1].itemNode, reinterpret_cast<ArkUI_NodeHandle>(0x9502));

    const MockArkUINativeProvider::SetAttributeRecord* firstGridItemHeightPolicy =
        FindLastAttributeForNode(*mockArkUIPtr_, component.gridItems_[0].itemNode, NODE_HEIGHT_LAYOUTPOLICY);
    ASSERT_NE(firstGridItemHeightPolicy, nullptr);
    ASSERT_FALSE(firstGridItemHeightPolicy->values.empty());
    EXPECT_EQ(firstGridItemHeightPolicy->values[0].i32,
        A2UIArkUITypeConverter::ToArkUILayoutPolicy(A2UILayoutPolicy::WRAP_CONTENT));

    mockArkUIPtr_->resetAttributeRecords_.clear();
    ArkUINodeApiAdapter componentApplier = CreateNodeApiAdapter(component);
    std::unique_ptr<JsonAdapter> explicitRowsTemplateStyles = ParseJson(R"({ "rowsTemplate": "40 40" })");
    ASSERT_NE(explicitRowsTemplateStyles, nullptr);
    component.ApplyComponentSpecificStyles(explicitRowsTemplateStyles->GetRoot(), componentApplier);
    bool didResetFirstGridItemHeightPolicy = false;
    for (const auto& record : mockArkUIPtr_->resetAttributeRecords_) {
        if (record.nodeHandle == component.gridItems_[0].itemNode && record.attribute == NODE_HEIGHT_LAYOUTPOLICY) {
            didResetFirstGridItemHeightPolicy = true;
        }
    }
    EXPECT_TRUE(didResetFirstGridItemHeightPolicy);

    ArkUI_NodeHandle moveView = component.nativeView_;
    component.nativeView_ = nullptr;
    component.OnMoveChild(childA, 0, 1);
    component.nativeView_ = moveView;
    ASSERT_EQ(component.gridItems_.size(), 2U);
    EXPECT_EQ(component.gridItems_[0].child.lock(), childB);
    EXPECT_EQ(component.gridItems_[1].child.lock(), childA);

    component.OnRemoveChild(std::make_shared<A2UIComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x9403), false));
    component.OnRemoveChild(childA);
    ASSERT_EQ(component.gridItems_.size(), 1U);
    EXPECT_EQ(component.gridItems_[0].child.lock(), childB);

    component.nativeView_ = nullptr;
    component.OnRemoveChild(childB);
    EXPECT_TRUE(component.gridItems_.empty());
    component.nativeView_ = moveView;
}

TEST_F(ExtendedGridComponentTddTest, should_only_attach_grid_adapter_when_lazy_mode_is_ready)
{
    ExtendedGridComponent component;
    auto adapterNode = std::make_shared<GridAdapterNode>();

    mockArkUIPtr_->setAttributeRecords_.clear();
    component.SetLazyMode(false);
    EXPECT_EQ(FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_NODE_ADAPTER), nullptr);

    component.SetLazyMode(true);
    EXPECT_EQ(FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_NODE_ADAPTER), nullptr);

    component.mode_ = ExtendedGridComponent::Mode::EAGER;
    component.SetAdapterNode(adapterNode);
    EXPECT_EQ(FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_NODE_ADAPTER), nullptr);

    mockArkUIPtr_->setAttributeRecords_.clear();
    component.SetLazyMode(true);
    const MockArkUINativeProvider::SetAttributeRecord* lazyModeRecord =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_NODE_ADAPTER);
    ASSERT_NE(lazyModeRecord, nullptr);

    mockArkUIPtr_->setAttributeRecords_.clear();
    component.SetAdapterNode(adapterNode);
    const MockArkUINativeProvider::SetAttributeRecord* adapterRecord =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_NODE_ADAPTER);
    ASSERT_NE(adapterRecord, nullptr);
}

TEST_F(ExtendedGridComponentTddTest, should_cover_setup_lazy_adapter_success_and_failure_paths)
{
    ArkUI_NativeNodeAPI_1* nativeNodeApi = ArkUINativeAPI::GetInstance().GetNativeNodeAPI();
    ASSERT_NE(nativeNodeApi, nullptr);

    std::unique_ptr<JsonAdapter> templateDescriptor =
        ParseJson(R"({"id":"gridTemplate","component":"Text","content":"row"})");
    std::unique_ptr<JsonAdapter> items = ParseJson(R"([
        {"label":"row-0"},
        {"label":"row-1"}
    ])");
    ASSERT_NE(templateDescriptor, nullptr);
    ASSERT_NE(items, nullptr);

    ChildListDescriptor absolutePathList = BuildTemplateChildListDescriptor("gridTemplate", "/items");
    ChildListDescriptor relativePathList = BuildTemplateChildListDescriptor("gridTemplate", "items");

    {
        ExtendedGridComponent component;
        SurfaceSlot localSlot;
        component.nativeView_ = nullptr;
        EXPECT_FALSE(component.SetupLazyAdapter(absolutePathList, localSlot));
    }

    {
        ExtendedGridComponent component;
        SurfaceSlot localSlot;
        EXPECT_FALSE(component.SetupLazyAdapter(absolutePathList, localSlot));
    }

    {
        ExtendedGridComponent component;
        SurfaceSlot localSlot;
        localSlot.allComponentDescriptorStore_["gridTemplate"] = templateDescriptor->GetRoot();
        localSlot.bindingEngine_ = nullptr;
        EXPECT_FALSE(component.SetupLazyAdapter(absolutePathList, localSlot));
    }

    {
        ExtendedGridComponent component;
        SurfaceSlot localSlot;
        localSlot.allComponentDescriptorStore_["gridTemplate"] = templateDescriptor->GetRoot();
        EXPECT_TRUE(component.SetupLazyAdapter(absolutePathList, localSlot));
    }

    {
        ExtendedGridComponent component;
        SurfaceSlot localSlot;
        localSlot.allComponentDescriptorStore_["gridTemplate"] = templateDescriptor->GetRoot();
        ScopedSetAttributeOverride setAttributeOverride(nativeNodeApi, ReturnSetAttributeFailure);
        EXPECT_FALSE(component.SetupLazyAdapter(relativePathList, localSlot));
        EXPECT_FALSE(component.IsLazyMode());
    }

    {
        ExtendedGridComponent component;
        SurfaceSlot localSlot;
        localSlot.allComponentDescriptorStore_["gridTemplate"] = templateDescriptor->GetRoot();
        mockArkUIPtr_->setAttributeRecords_.clear();
        EXPECT_TRUE(component.SetupLazyAdapter(relativePathList, localSlot));
        ASSERT_NE(component.GetAdapterNode(), nullptr);
        EXPECT_TRUE(component.IsLazyMode());
        EXPECT_EQ(component.GetAdapterNode()->GetDataPath(), "items");
        EXPECT_NE(FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_NODE_ADAPTER), nullptr);
    }

    {
        ExtendedGridComponent component;
        SurfaceSlot localSlot;
        localSlot.allComponentDescriptorStore_["gridTemplate"] = templateDescriptor->GetRoot();
        std::shared_ptr<DataModel> dataModel = localSlot.GetOrCreateDataModel();
        ASSERT_NE(dataModel, nullptr);
        dataModel->UpdateByPath("/items", items->GetRoot());

        EXPECT_TRUE(component.SetupLazyAdapter(absolutePathList, localSlot));
        std::shared_ptr<GridAdapterNode> firstAdapter = component.GetAdapterNode();
        ASSERT_NE(firstAdapter, nullptr);
        EXPECT_EQ(firstAdapter->GetDataModel(), dataModel);
        EXPECT_EQ(firstAdapter->GetDataPath(), "/items");

        EXPECT_TRUE(component.SetupLazyAdapter(absolutePathList, localSlot));
        EXPECT_EQ(component.GetAdapterNode(), firstAdapter);
    }
}

TEST_F(ExtendedGridComponentTddTest, should_fallback_to_eager_template_expansion_when_lazy_setup_fails)
{
    ExtendedGridComponent component;
    ChildListDescriptor childList = BuildTemplateChildListDescriptor("missingTemplate", "/items");
    std::list<std::string> childIds = { "stale-child" };

    EXPECT_FALSE(component.ExpandTemplateChildren(childList, slot_, childIds));
    EXPECT_TRUE(childIds.empty());
    EXPECT_FALSE(component.IsLazyMode());
}

} // namespace
