# ALT 页签接入大模型生成（TaskSpec / ALT+ASC）方案

> 状态：已实施（2026-08-06，见 `index.html` / `app.js` / `add-ui.css` / `scripts/alt-prompts/`）
> 关联：`docs/TaskSpec.md`、`.agents/skills/harmony-card-generation-datamodel-first/SKILL.md`、`docs/alt-protocol-pyodide-plan.md`

## 1. 目标

在左侧「ALT 生成」页签内闭环完成卡片生成全流程：

```
用户 query → [生成 TaskSpec] → TaskSpec 文本 → [生成 ALT/ASC] → ALT/ASC 文本 → [编译并渲染]（现有 Pyodide 管线）
```

- 两个「生成」按钮都调用用户自配的 chat/completions 大模型（BYOK：BaseURL + Key 存在用户浏览器，Key 加密存储，不出浏览器）。
- 生成 TaskSpec 的上下文模板重点引用素材库与点击事件能力文档，素材强制为本地 SVG 白名单。
- 渲染/编译告警从左侧栏迁移到中间画布下方，支持折叠。
- 页面右上角新增设置入口，管理 BaseURL / API Key / 模型名，Key 采用「WebCrypto + 用户口令派生密钥」加密落盘。

## 2. 可行性结论

**可行，风险可控。** 关键前提已实测：

- GitHub Pages 是纯静态托管，但浏览器可直接 `fetch` 外部 OpenAI 兼容 API（无需任何后端）。
- CORS 预检实测（无鉴权 OPTIONS）：OpenAI、DeepSeek、Moonshot、智谱、通义 DashScope、SiliconFlow 均返回 `Access-Control-Allow-Origin` 并放行 `authorization` / `content-type` 请求头。
- 浏览器端 `crypto.subtle`（PBKDF2 + AES-GCM）原生支持，无需第三方库即可实现口令派生加密。

## 3. 页面改动

### 3.1 ALT 页签（左栏）结构调整

```
┌────────────────────────────────┐
│ 用户 query（多行输入框）          │
│ [生成 TaskSpec]  ← 调用 LLM #1  │
│ ────────────────────────────── │
│ TaskSpec（JSON，可手改）          │
│ [生成 ALT/ASC]  ← 调用 LLM #2   │
│ ────────────────────────────── │
│ ALT 结构（card.alt.txt）         │
│ ASC 语义（card.asc.txt）         │
│ 主题 / 宽度 / 高度               │
│ [编译并渲染]（现有，不改）          │
└────────────────────────────────┘
```

- 生成中：按钮禁用 + 局部 loading 文案（或复用现有加载遮罩样式），不阻塞页面其他操作。
- 生成失败：在 ALT 面板内展示错误（模型报错 / 超时 / 输出解析失败），不写入输入框。
- 生成成功：结果自动填入对应输入框，用户可手改后继续。
- 第二个按钮的输入取「TaskSpec 输入框当前内容」（用户可能手改过）。

### 3.2 告警信息迁移到画布下方（中栏）

- 新增 `#renderWarnings` 区域，位于 `#stage` 之后（`stage-hint` 之前），可折叠：
  - 头部：标题 + 告警数量角标 + 展开/收起箭头；
  - 主体：告警列表（复用的错误/警告样式）。
- 数据源统一汇入：`parseJsonl` 的解析告警、`renderInput` 的渲染告警、ALT 编译产生的 `warnings`（原 `altReport`）。
- 左栏 `#altReport` 移除或仅在生成错误时使用（生成错误留在左栏更贴近操作点，渲染告警去画布下方——评审点 5）。
- 折叠状态可记忆（localStorage），默认：无告警时收起，有告警时展开。

### 3.3 右上角设置按钮 + 二级配置弹窗

- 位置：中栏 `stage-toolbar` 右上角（缩放按钮左侧）新增 ⚙ 设置按钮。
- 弹窗字段：
  - BaseURL（如 `https://api.deepseek.com/v1`；说明：请求实际发往 `${baseURL}/chat/completions`，若用户填到 `/chat/completions` 做去重容错）
  - 模型名（如 `deepseek-chat`）
  - API Key（`type=password`）
  - 主口令（新建或输入，仅用于派生加密密钥，不落盘）
  - 确认口令
  - 按钮：保存 / 清除配置
- 保存时校验：BaseURL 非空、Key 非空、口令与确认一致且口令非空。

## 4. 大模型调用设计（BYOK）

### 4.1 请求

```js
POST ${baseURL}/chat/completions
Authorization: Bearer <解密后的 Key（仅内存）>
Content-Type: application/json
{
  "model": "<模型名>",
  "messages": [
    { "role": "system", "content": "<模板渲染结果，见 §6>" },
    { "role": "user", "content": "<用户 query 或 TaskSpec 文本>" }
  ],
  "temperature": 0.3
}
```

- 超时：`AbortController` 60s；`429` 可选重试 1 次（指数退避）。
- 错误提示：网络错误 / 401（Key 或 BaseURL 错误）/ 429（限流）/ 4xx-5xx，中文文案 + 排查指引。
- 输出解析：
  - TaskSpec：提取 ```json 代码块（或首个 `{` 到末尾 `}`），`JSON.parse` 后做字段级校验（见 §6.2）。
  - ALT/ASC：提取 `<alt>...</alt>` 与 `<asc>...</asc>`（与 `scripts/run_alt_dataset.py` 的解析规则一致），失败时提示重试。
- 流式输出（SSE）：**v1 不做**，调用函数预留 `stream` 参数与解析接口，后续可选开启。
- 安全：Key 只进入 `Authorization` 头；不进 URL、不写日志、不打印 console。

### 4.2 CORS 实测记录（2026-08-06，无鉴权 OPTIONS 预检）

| 厂商 | 预检状态 | ACAO | 允许方法 | 允许头 |
| --- | --- | --- | --- | --- |
| OpenAI | 200 | 回显来源 | GET/OPTIONS/POST | authorization, content-type |
| DeepSeek | 200 | 回显来源 | POST | authorization, content-type |
| Moonshot | 204 | 回显来源 | POST | authorization, content-type |
| 智谱 | 200 | 回显来源 | POST | authorization, content-type |
| 通义 DashScope | 200 | `*` | POST/GET | authorization, content-type |
| SiliconFlow | 204 | `*` | POST | `*` |

> 个别未实测厂商若不放行 CORS，浏览器直调会被拦截；届时在设置中预留「可选 CORS 代理地址」字段作为兜底（v2，评审点 3）。

## 5. Key 加密存储设计（WebCrypto）

### 5.1 加密流程（保存时）

1. 用户输入主口令 `P`（不落盘）。
2. 生成随机 `salt`（16B）与 `iv`（12B）。
3. `PBKDF2(P, salt, SHA-256, 310000 次)` 派生 AES-GCM 256 密钥。
4. 用该密钥加密 API Key，得到密文。
5. 写入 `localStorage['a2ui.llm.v1']`：

```json
{
  "v": 1,
  "baseURL": "https://api.deepseek.com/v1",
  "model": "deepseek-chat",
  "kdf": { "algo": "PBKDF2", "hash": "SHA-256", "iterations": 310000, "salt": "<base64>" },
  "aead": { "algo": "AES-GCM", "iv": "<base64>", "data": "<base64 密文>" }
}
```

- `baseURL` / `model` 非敏感，明文存储；Key 仅存密文。

### 5.2 解密流程（每次会话首次调用时）

1. 内存变量 `plainKey` 为空 → 弹出主口令输入框（复用设置弹窗的口令字段）。
2. 用口令 + 存储的 salt 派生密钥 → AES-GCM 解密。
3. 成功：明文 Key 只留在内存模块变量中，会话内不再询问；页面刷新/关闭后自动清空。
4. 失败（口令错误）：提示重新输入；不落任何明文。

### 5.3 边界与提示

- **不可恢复**：口令丢失后密文无法解密，只能「清除配置」重新设置（弹窗内明示）。
- 真实防护范围：防明文落盘、防误导出/截图、防同一设备其他系统用户直接读取；**不防本页 XSS 或恶意浏览器扩展**（缓解手段：HTTPS、CSP 收紧、不引入第三方脚本、控制台不打印）。
- 不提供「明文存储」开关（评审点 4），避免误用。

## 6. 两段提示词模板（独立文件，便于修改与审阅）

### 6.1 存放位置

建议单一来源 `scripts/alt-prompts/`（随 GitHub Pages 发布，前端 `fetch` 加载；同时就是审阅文档）：

```
scripts/alt-prompts/
  task-spec-generation.md   # 第一段：userQuery → TaskSpec
  alt-asc-generation.md     # 第二段：TaskSpec → ALT + ASC
```

> 评审点 1：如希望模板不进发布产物，可改放 `docs/` 并在前端构建期注入；推荐前者（零构建、单一来源）。

### 6.2 模板一：`task-spec-generation.md`（query → TaskSpec）

结构：角色定义 → 输入占位 `{userQuery}` → 输出契约 → 硬约束 → 参考附录 → 简短示例。

硬约束（源自 `docs/TaskSpec.md` 与 skill）：

- 顶层**只允许**五个字段：`userQuery`、`size`、`eventCandidates`、`dataModelSchema`、`assetCandidates`；禁止 `displayCandidates`、`role`、`cardSpec`、`rules`、`dataModel` 等非协议字段。
- `size`：默认 `2x2`，只有受保护文本/热区/并列关系/关键媒体明确放不下才升级 `2x4`。
- `dataModelSchema` 节点必须含 `type`、`description`、`sampleValue`；`sampleValue` 必须脱敏且贴近 UI 展示（如 `"26℃"`、`"多云"`）。
- `assetCandidates`：**只允许从素材库白名单选择**（`reference/design/asset-library.md`，`src` 统一为 `resources/base/media/*.svg`）；禁止 PNG、网络图、内联/base64 SVG、emoji、占位媒体；`description` 必须非空，说明视觉语义、适用场景与主配色（SVG 可支持主题改色）。
- `eventCandidates`：按 `reference/capability/event-capability/click-event.md` 匹配能力（`clickToCallPhone` / `clickToDeeplink` / `clickToIntent`），`args` 只填对应参数；禁止出现 `id`、`label`、`description`、`required`、`onClick` 字段。
- 模板不输出布局、组件、字号、颜色决策（那是第二阶段 ALT 模型的事）。

参考附录注入方式：

- `asset-library.md`（89 行素材表）**全文注入**，保证模型只从表内选 `src`；
- `click-event.md`（能力速查 + Deeplink/Intent 目标表）**全文注入**，保证事件能力与参数规范一致；
- 两文件内容变化时只需更新模板附录，不涉及前端代码。

### 6.3 模板二：`alt-asc-generation.md`（TaskSpec → ALT + ASC）

结构：角色定义 → 输入占位 `{taskSpec}` → 输出契约 → 布局/绑定/素材/事件约束 → 降级顺序 → 简短示例。

硬约束（源自 skill `SKILL.md` 一致性约定）：

- 输出两段：`<alt>...</alt>`（自动布局 ALT 文本，`Column root card=2x2 theme=neutral-light` 形式）+ `<asc>...</asc>`（语义文本），与 `run_alt_dataset.py` 解析格式一致。
- 尺寸：`2x2 = 140x140`、`2x4 = 300x140`，root `padding: 12`、`borderRadius`（18/22）、`clip: true`，内部组件一律数值宽高，不用 `matchParent`。
- 绑定：静态值或完整 `{{ ... }}` 表达式，路径必须能从 `dataModelSchema` / CardSpec 推导；`sampleValue` 作为 `updateDataModel` 默认值保证直接渲染美观。
- 素材：只用 TaskSpec `assetCandidates` 白名单 SVG；SVG 颜色由主题映射写入 `Image.styles.fillColor`，模型不选色。
- 事件：`onClick` 只能由 `eventCandidates` 转换，参数严格按 `click-event.md`（如拨号 `phoneNumber`、导航 `trafficpe` 取值 `Drive|Walk|Cycle|Bus`、Deeplink 目标表原样复制）。
- 语义 ID 稳定：`surface_card`、`root`、`header_row`、`title_text`、`primary_value`、`primary_caption`、`action_button` 等。
- 布局失败降级顺序：缩短弱文本 → 删除可选槽位 → 降字号阶梯 → 拆行/改 Column → 升级 `2x4` → 能力边界说明。

### 6.4 模板加载

- 前端在生成按钮点击时 `fetch('scripts/alt-prompts/xxx.md')`，把 `{userQuery}` / `{taskSpec}` 占位替换为实际内容，再作为 `system` 消息发送。
- 模板文件改动直接生效（GitHub Pages 发布后），无需改代码。

## 7. 数据流

```
用户 query
  │  [生成 TaskSpec]
  ▼
模板一 + asset-library 全文 + click-event 全文 ──► chat/completions ──► TaskSpec JSON
  │  字段级校验（顶层五字段/素材白名单/事件能力）
  ▼
填入 TaskSpec 输入框（用户可手改）
  │  [生成 ALT/ASC]
  ▼
模板二 + 校验后的 TaskSpec ──► chat/completions ──► <alt> + <asc>
  ▼
填入 ALT/ASC 输入框 ──► [编译并渲染]（现有 Pyodide 管线）──► DSL + 渲染
  ▼
告警 → 画布下方可折叠区域
```

## 8. 改动文件清单（评审通过后）

| 文件 | 改动 |
| --- | --- |
| `index.html` | ALT 面板加 query 框 + 两个生成按钮；`stage` 下方加 `#renderWarnings`；`stage-toolbar` 加设置按钮；新增设置 modal DOM |
| `add-ui.css` | 生成按钮、告警区（可折叠）、设置弹窗、口令输入样式 |
| `app.js` | 新增：LLM 配置读写 + WebCrypto 加密解密模块、`chatCompletion()` 调用、两段生成流程、输出解析与校验、告警统一迁移；修改：`renderInput`/ALT 编译告警写入 `#renderWarnings` |
| `scripts/alt-prompts/task-spec-generation.md` | 模板一（新增） |
| `scripts/alt-prompts/alt-asc-generation.md` | 模板二（新增） |
| `docs/alt-llm-generation-plan.md` | 本文档 |

## 9. 风险与对策

| 风险 | 影响 | 对策 |
| --- | --- | --- |
| 大模型输出格式不稳定 | TaskSpec/ALT 解析失败 | 提取代码块容错 + 字段级校验 + 明确错误提示 + 可手动修改；必要时重试 1 次 |
| 个别厂商未放行 CORS | 浏览器直调被拦 | 已实测 6 家主流放行；预留「CORS 代理地址」可选字段（v2） |
| 口令丢失 | 无法解密 Key | 清除配置重新设置，弹窗明示不可恢复 |
| 加密安全边界被高估 | 误以为绝对安全 | 设置弹窗内写明真实防护范围（防明文落盘/导出，不防 XSS/扩展） |
| 模板过长消耗 token | 生成慢/超限 | 素材表与事件表全文仅约 157 行，可接受；必要时压缩为索引 + 关键字段 |
| 浏览器存储 5MB 限制 | Key/配置存不下 | 只存配置与密文（KB 级），模板不进 storage（运行时 fetch） |

## 10. 实施阶段（评审后拆分）

- P0：设置弹窗 + WebCrypto 加密/解密 + 会话口令流程。
- P1：「生成 TaskSpec」+ 模板一 + 解析与字段校验。
- P2：「生成 ALT/ASC」+ 模板二 + `<alt>/<asc>` 解析。
- P3：告警迁移到画布下方可折叠区域。
- P4：模板审阅打磨 + 用 `tmp/run_luna_v4` 代表性用例做端到端回归（query → 生成 → 编译渲染）。

## 11. 待评审问题

1. 模板存放：`scripts/alt-prompts/`（随站发布，推荐）还是 `docs/`（仅文档）？
2. 是否要求流式输出（SSE）？建议 v1 不做。
3. 是否需要「CORS 代理地址」字段兜底？建议 v2 再加。
4. 是否允许「明文模式」开关（仅测试用）？建议不允许。
5. 告警区：渲染/编译告警统一去画布下方，生成错误留在左栏贴近操作点——是否认可？
6. 口令策略：会话内免重复输入（默认）是否需要"每次调用都要口令"的严格模式？
7. 设置弹窗是否顺带支持多个厂商预设（BaseURL+默认模型名下拉）？
