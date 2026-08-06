# GenUI 渲染器更新同步文档（2026-08-06）

> 基线提交：`8a2c9fc 更新GenUI渲染器源码`（对比 `7e381f9`）
> 范围：`references/genui/` 的 C++/ArkTS 渲染核心、schema、函数库，以及同提交内的 ALT/脚本改动。
> 目标：让 `genui-renderer.js`（Designer 与 CLI 共享的 Web 渲染核心）的行为与 GenUI 运行时保持一致。

## 1. 提交概况

`8a2c9fc` 共改动 311 个文件（+28,394 / -9,096），主要内容分为四类：

| 类别 | 说明 | 与 Web 渲染器的关系 |
| --- | --- | --- |
| 渲染核心（C++/ETS） | 表达式引擎、动态值解析、原生函数库、样式解析、组件行为 | 直接相关，本次同步重点 |
| Schema | `rawfile/schema/` 组件与函数契约更新 | 间接相关，用于确认字段形态 |
| ALT 训练/评估 | `docs/alt-*.md`、`scripts/alt_converter.py` 等 | 训练侧协议，不属于运行时渲染 |
| 平台桥接 | ArkUI Node/OH API Adapter、SurfaceSlot、错误分发 | Web 端以 CSS/DOM 等价实现，不逐项搬运 |

## 2. 更新点清单（渲染行为）

### 2.1 表达式引擎

- `&&` / `||` 短路后返回操作数本身（不再是强转布尔），与 JS 语义一致；
- 函数参数求值改为“任一参数 undefined 则整体 undefined”，不再提前中断；
- `size()` 内置函数：非数组参数返回 `0` 并报错，数组返回长度；
- 新增数据模型 JSON Pointer 方括号语法恢复：`$__dataModel["/a/b"]` 按 JSON Pointer 解析（不再当作普通对象键）；
- 未定义/非绝对变量引用视为非法表达式，属性回落默认值。

对应文件：`expression/Lexer.cpp`、`expression/Parser.cpp`、`expression/Evaluator.cpp`、`expression/ExpressionEngine.cpp`。

### 2.2 动态值解析管线

运行时对属性值统一递归解析，支持四种形态（`data/DynamicValueResolver.cpp`）：

| 形态 | 示例 | 语义 |
| --- | --- | --- |
| 表达式 | `"{{ size(${/items}) }}"` | 表达式求值，`${...}` 作为 JSON Pointer 数据引用 |
| 模板字符串 | `"共 ${/total} 条"` | 逐段替换 `${/path}`；`\${` 转义为字面 `${`；未闭合保留原样 |
| 路径绑定 | `{"path": "/a/b"}` | 从 DataModel 按 JSON Pointer 读取 |
| 函数调用 | `{"call":"formatNumber","args":{...}}` | 递归解析参数后调用原生函数注册表 |

对象、数组会递归进入；解析失败时属性回落默认值。

### 2.3 原生函数库（本次新增 14 个）

注册表见 `functions/NativeFunctionRegistry.cpp` 与 `functions/impl/`：

| 函数 | 参数（均需先解析） | 行为要点 |
| --- | --- | --- |
| `required` | `value` | 非 null、非空字符串、非空数组/对象则为 true |
| `regex` | `value`, `pattern` | 全量正则匹配（`regex_match`） |
| `length` | `value`, `min?`, `max?` | 字符串长度区间校验 |
| `numeric` | `value`, `min?`, `max?` | 数值区间校验 |
| `email` | `value` | 邮箱格式全量匹配 |
| `formatString` | `value` | 模板解析：`${/path}` 与 `${funcName(args)}` 嵌套调用 |
| `formatNumber` | `value`, `decimals?`, `grouping?` | 默认 2 位小数；grouping 默认 false |
| `formatCurrency` | `value`, `currency`, `decimals?`, `grouping?` | 输出 `CUR 123.45` |
| `formatDate` | `value`, `format` | ISO-8601 日期 + TR35 模式令牌 |
| `pluralize` | `value`, `zero/one/two/few/many/other` | CLDR 复数规则，按语言环境取类别 |
| `and` | `values[]` | 至少 2 个布尔且全 true |
| `or` | `values[]` | 至少 2 个且任一 true |
| `not` | `value` | 非布尔值返回 true |

### 2.4 样式

- 公共样式新增 `constraintSize`（`minWidth/maxWidth/minHeight/maxHeight`，支持 vp/fp/percent）；
- 新增 `visibility`（`visible|hidden|none`）；
- 新增 `backgroundImageSize`（`width/height`，支持 vp/fp/percent）；
- `linearGradient` 新增 `angle`（自定义角度）、`repeating`、显式 `stops` 覆盖；未提供 stop 时按 `index/(n-1)` 均分；
- 解析失败/非法样式回落默认值并报 schema warning。

对应文件：`styles/StyleApplyUtilsEffects.cpp`、`components/extended/ExtendedStyleResolver.cpp`、`ExtendedCommonStyleModifier.ets`。

### 2.5 组件行为

- Text：`text` 成为 `content` 的别名（两者任一即可）；`textOverflow` 未配合有限 `maxLines` 时告警；
- Image：`aspectRatio` 支持动态值/表达式，默认 1.0；
- Progress：新增 `styles.strokeWidth`（默认 4vp），linear 轨道厚度与 ring 描边共用；
- Checkbox：`styles.mark.size / strokeWidth / strokeColor` 支持动态值（path/表达式）；
- Grid/List：模板适配器重构、`fr` 栅格模板校验（Web 端未实现 Grid/Repeat，仅记录）；
- 主题：卡片圆角/内边距/边框不再按断点分支，统一默认值（`CardTheme.cpp`）。

### 2.6 Schema（Extended）

- `DynamicValue` 统一为“表达式 / 路径绑定 / 函数调用”三种引用的并集；
- 新增 `ExtendedColumn.json` schema；`ExtendedRow/Grid/List/Tabs/TabContent` 字段更新；
- 新增 `Extended/functions/setAttributes.json`、`setDataModel.json`、`break.json`。

## 3. JS 渲染器同步内容（本次已实施）

以下改动已落入 `genui-renderer.js`：

1. **动态值解析管线**：`evaluateBinding` 扩展为递归解析器，覆盖表达式、`${...}` 模板（含 `\${` 转义）、`{"path": ...}` 路径绑定、`{"call": ...}` 函数调用，数组/对象递归；
2. **原生函数注册表**：实现上述 14 个函数，参数先递归解析，行为对齐 C++ 测试用例（默认 2 位小数、无分组等）；
3. **表达式增强**：`$__dataModel["/a/b"]` 方括号 JSON Pointer 语法；表达式失败回落（不再渲染原始 `{{ }}` 文本）；
4. **样式**：`constraintSize`、`visibility`、`backgroundImageSize`、`linearGradient` 的 `angle/repeating/stops`；
5. **组件**：Text `text` 别名、Image `styles.aspectRatio`、Progress `styles.strokeWidth`、Checkbox `mark` 动态成员。

未同步（Web 端不适用或超出当前范围）：ArkUI 原生节点适配、Surface 错误分发、Grid/List 模板适配器、ALT 训练协议、schema warning 上报（JS 端仅在控制台输出）。

## 4. 验证

- `node --check genui-renderer.js` 与 `git diff --check`；
- Node 冒烟脚本覆盖：表达式、模板、路径绑定、函数调用与 14 个原生函数的行为断言；
- 使用 `references/testset/` 样例（如 `Case-01-low-power-001`、`Case-09-weather-care-001`）通过 Designer/CLI 渲染，对照 `card.dsl.png`。
