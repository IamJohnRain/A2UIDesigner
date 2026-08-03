# A2UI C++ TDD 测试框架

本目录是 `genui/src/test/cpp` 的活动基础设施层，供 `suites/` 下的测试复用。

## 内容

```text
infra/
├── TestFixture.h
├── include/gtest_ext.h
├── mock/
│   ├── mock_napi_provider.h/.cpp
│   ├── mock_arkui_native_provider.h/.cpp
│   └── stub_impl.cpp
└── README.md
```

## 角色

- `TestFixture.h`
  提供 `A2UITest`，自动注入 `MockNapiProvider` 与 `MockArkUINativeProvider`
- `include/gtest_ext.h`
  提供 `testing::ext::TestSize` 与兼容三参数 `TEST_F`
- `mock/`
  提供 NAPI / ArkUI 的 mock 与 C 接口 stub

## 使用方式

- 测试源码统一放在 `../suites/`
- 新增用例时 include：
  `#include "TestFixture.h"`
- 如果需要 Level 标注，再 include：
  `#include "gtest_ext.h"`

构建与运行方式见上级目录 [README.md](../README.md)：

- `构建`
- `运行测试`
  这里明确区分了“全部测试套”、“单个测试套”、“单个测试用例”
- `覆盖率统计`
