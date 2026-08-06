# ALT 转换器（alt_converter.py）底层逻辑深度分析报告

> 分析对象：`scripts/alt_converter.py` + `scripts/config/alt-layout-profile.json` + `scripts/config/alt-tuning.json` + `scripts/config/alt-themes.json`
> 结论先行：**卡片"简单朴素"和"复杂卡片转换失败"不是 bug，而是该转换器的确定性设计使然**。它面向"小模型输出紧凑意图 → 编译器推导全部样式"的场景，用极窄的能力预算换取"绝不溢出、绝不重叠、颜色可控"。要改变现状，需要先理解它的能力边界从哪来、失败在哪触发。

---

## 1. 转换管线总览（t2d：TaskSpec + ALT + ASC → DSL）

编译流程共 8 步（对应 `main()` → `auto_layout_document()` → `alt_to_dsl()`）：

| 步骤 | 代码位置 | 作用 |
| --- | --- | --- |
| 1. 语法校验 | `parse_alt` / `validate_auto_protocol` | 缩进、属性白名单、card/theme、层级、容量 |
| 2. 语义校验 | `validate_auto_asc` | ASC 节点映射、SVG/事件索引、绑定路径存在性 |
| 3. 生成样例数据 | `sample_data_model` | 从 TaskSpec `dataModelSchema.sampleValue` 递归生成 |
| 4. 解析文案 | `auto_semantic_text` | 求值动态绑定/expr，得到可测量的静态文案 |
| 5. 测量固有尺寸 | `measure()` | 按角色/组件计算 preferred 与 minimum 宽高 |
| 6. 空间分配 | `layout()` + `allocate_axis` | 自顶向下分配 Row/Column 空间，整数化取整 |
| 7. 应用主题 | `apply_alt_styles` + `theme_values` | 生成具体颜色、渐变、圆角、fillColor |
| 8. 静态检查 | `validate_layout` + `build_layout_report` | 任何 hard error 存在则**不输出 DSL** |

关键特性：**第 8 步是"全有或全无"**——`auto_layout_document` 收集的 `error` 级 issue 会使编译直接失败（`raise ConversionError`），不会输出明知有问题的 DSL。

---

## 2. 元素能力边界（profile 硬编码，模型不可越界）

### 2.1 画布

| 规格 | 画布 | root padding | radius |
| --- | --- | --- | --- |
| 2x2 | 140 x 140vp | 12vp | 18vp |
| 2x4 | 300 x 140vp | 12vp | 22vp |

**可用内容区固定为画布减 padding**：2x2 内区 116x116vp，2x4 内区 276x116vp。这是所有溢出判断的基准。

### 2.2 容量硬上限（`limits`）

| 约束 | 2x2 | 2x4 |
| --- | ---: | ---: |
| 最大层级 | 4 | 5 |
| 最大节点数 | 10 | 18 |
| 最大 Text | 4 | 8 |
| 最大 Button | 1 | 2 |
| 最大 Image | 2 | 3 |
| 最大 Progress | 1 | 2 |
| 最大 Checkbox | **0** | 3（不参与自动训练） |
| 最大 List/Repeat | **0** | 1（不参与自动训练） |
| **可见文本预算（单位）** | **32** | 72 |
| 容器直接子节点 | ≤3 | ≤3 |
| role=primary 节点 | ≤1 | ≤1 |

### 2.3 字号与行数（`typography` + `textRules`）

| 角色 | regular 字号/字重 | compact 字号/字重 | 自适应下限（步进） | 最大行数 |
| --- | --- | --- | --- | --- |
| title | 14/600 | 14/600 | 12（-2） | 2 |
| primary | **32/500** | 20/500 | **16（-2）** | **1** |
| status | 14/500 | 14/500 | 12（-2） | 1 |
| warning / error | 14/500 | 14/500 | 12（-2） | 1 |
| metric | 20/500 | 18/500 | 14（-2） | 1 |
| support | 12/400 | 12/400 | 10（-1） | 2 |
| meta | 10/400 | 10/400 | 8（-1） | 2 |
| action | 14/500 | 14/500 | 12（-2） | 1 |

注意 `auto_font()` 的 primary 特殊规则：按文本单位动态降级——`>12 单位→16fp`、`>8→18fp`、`>3→20fp`，含时间词（"分钟/小时/天"或 `:`）直接压到 20fp。**primary 永远单行、最少 16fp**。

行数规则：`configured_text_max_lines` 只有 `role 配置 >1 且 text_units > 6` 才允许换行（即 title/support/meta 的短文本仍然是单行）。

### 2.4 文本测量公式（确定性，非真实字体测量）

```python
text_units: 全角/宽字符=1.0，大写=0.68，数字=0.62，小写=0.56，空白=0.35，其他=0.45
estimated_text_width = text_units × font_size × weight_factor × safety(1.10)
weight_factor = 1 + max(0, weight - 400) / 5000
行高 = font_size + 4
```

含义：**1 个中文字 ≈ 1 单位 ≈ 1×font_size×1.1 的宽度**。2x2 的 32 单位文本预算 ≈ 32 个中文字（约 2~3 个短句）。

### 2.5 组件固有尺寸（`alt-tuning.json` 的 `components`）

> 组件几何超参（Button/Checkbox/Image/Progress/Divider）已从 `alt-layout-profile.json` 迁出，统一放在 `scripts/config/alt-tuning.json`，含义见 `docs/alt-tuning-parameters.md`。

| 组件 | 规则 |
| --- | --- |
| **Button** | 固有宽 = max(48, 文本宽 + 2×12padding + 4safety)；固有高 = max(32, font+4+2×8)；`chars = floor((宽-24)/字号)`；radius=高/2（胶囊）；永远 protect |
| **Checkbox**（自动路径禁用） | 兼容路径外高 48vp 不可缩放；控件 20x20、margin 2、label gap 12、字号 16 |
| **Image** | 按角色固定正方形：asset=24 / meta=18 / support=20 / action=20 / primary(2x2)=48 / primary(2x4)=56；objectFit=contain |
| **Progress** | primary→ring 正方形 56(2x2)/64(2x4)；其他→linear 高 8vp、最小宽 64vp |
| **Divider** | Row 内=竖线，其余=横线；stroke 1px |
| **间距** | rootGap=8 / nestedGap=6 / denseGap=4；`density=compact` 或子节点≥3 时用 4 |

`density`（regular/compact）由三条件触发：规格 2x2、节点数 >7(2x2)/13(2x4)、文本单位 > 上限×0.72。**compact 会把 primary 从 32fp 压到 20fp、metric 从 20 到 18、default 从 14 到 12**。

### 2.6 主题（3 个，token 固定）

`neutral-light` / `ambient-light` / `focus-dark`。每个主题只有 8 组 token：root/panel 表面色与渐变、3 级文字色、icon 色、主按钮（bg+渐变）、进度条 fill/track、checkbox 选中/未选中/mark、divider、4 个 status 色。**模型能选择的只有主题名**；所有颜色、渐变、圆角由编译器从 token 生成。

---

## 3. 失败模式清单（hard error 触发点）

以下任一条件都会导致 `ConversionError`，**不输出 DSL**：

### 3.1 协议级（`validate_auto_protocol`）
1. root 非 `Column`、无 `card=2x2/2x4`、`card` 与 TaskSpec.size 不一致、theme 不在白名单；
2. **容量超限**：节点/Text/Button/Image/Progress/Checkbox/List 计数超 max；
3. **层级超限**：深度 > maxDepth；
4. **容器 >3 个直接子节点**；
5. **超过一个 primary**；
6. 叶子节点缺 role、空容器、出现 Stack；
7. 非 root 节点携带 card/theme 或任何推断属性（font/box/pad/bg 等一律非法）。

### 3.2 文本级（`auto_layout_document` 内）
8. **可见文本预算超限**：`visible_units > maxVisibleTextUnits`（2x2=32、2x4=72）——复杂卡片第一杀手；
9. **行数超限**：`required_lines > max_lines` 且角色为 title 或受保护单行角色（primary/status/warning/error/metric/action）→ error（support/meta 超行只 warning）；
10. **垂直空间不足**：文本要求高度 `(font+4)×lines` 超过可用高度。

### 3.3 组件级（`set_leaf_layout`）
11. **Button 溢出**：固有宽/高 > 可用空间；或 `alt_to_dsl` 阶段 label 字符数 > `chars` 容量；
12. **Checkbox 溢出**：固定高 48（或 22）> 可用空间；
13. **Image 缩水**：可用空间小于角色最小正方形（asset 24 / primary 48 等）；
14. **ring Progress 缩水**：小于 56/64vp 正方形；
15. **Row 最小值超限**：子最小宽之和 + gap > 内容宽（先尝试把 gap 压到 2，仍不够则报错）；
16. **Column 最小值超限**：同上按高度计算。

### 3.4 修复机制（`repair_*` + `prune_auto_low_priority_leaf`）
- 循环最多 **4 次**，每次只删**一个**最低优先级节点（Image>Progress(metric)>selection 文本>meta/support 文本）；
- 删到不能再删（无候选）仍失败 → 报错。**没有"全局重排"或"全局字号再优化"**。

---

## 4. 根本原因分析：为什么复杂卡片失败

### 原因 A：文本预算是最硬的天花板
`maxVisibleTextUnits` 把卡片限制在约 32（2x2）/72（2x4）个中文字单位。这是**设计意图**——协议明确要求"放不下就删低优先级事实"。任何"信息量大的卡片"（多个状态 + 说明 + 提示）都会撞上这个上限并**直接 error**，而不是被截断。复杂卡片 = 文案多 = 几乎必然失败。

### 原因 B：空间刚性分配，无溢出容忍
- 内容区是绝对预算：2x2 内区 116x116vp。一次典型垂直堆叠：2 行 title（36vp）+ primary（36vp）+ support（16vp）+ Button（32vp）+ 2 个 gap（12vp）≈ 132vp > 116vp → **溢出 error**；
- 受保护文本（title/primary/status/metric/action/warning/error）**不允许裁剪、省略、覆盖**，`overflow` 永远等于 `none`；
- 换行能力极其有限：只有 title/support/meta 可 2 行，且要求文本单位 >6。

### 原因 C：字号自适应区间太窄
`fit_text_font` 只在 `[minSize, preferred]` 之间按 step 降级（primary 32→16 步进 2）。**降到下限仍放不下就直接报错**，不存在"继续缩小 + 换行"的组合优化。长 primary 文本（>16 单位，比如 "86%" 很短没问题，但 "剩余电量 86% 预计可用 3 小时" 这种）必然失败。

### 原因 D：结构组合刚性
- 容器 ≤3 直接子节点 → 无法表达"header + 多组信息 + 多操作"的复杂结构；
- Row 内子项**无弹性收缩**：宽度 = 固有宽之和，超了先压 gap，再超就报错（自动路径不产生 `layoutWeight`/`flexShrink`）；
- 2x2 只有 1 个 Button、最多 2 个 Image、1 个 Progress；多事实只能合并或删减；
- 自动路径**完全禁用 Checkbox/List/Repeat/Stack**（2x2 连兼容入口都没有）→ 复杂交互（多选、列表、叠加）无法表达。

### 原因 E：修复机制浅层
`prune` 只是"删一个最低优先级节点"，循环 4 次；没有"垂直改水平布局""整体缩号重排""合并文本"等策略性修复。删除路径走完后依然失败，就失败。

### 原因 F：动态文案的测量不确定性
绑定路径在样例数据上求值失败时，用 `humanize_id` 语义占位（如 node_id `battery_value` → "数值"），真实文案可能与测量基准差异巨大——测量通过但真实渲染溢出（这是运行时问题，编译期不报）。

---

## 5. 为什么卡片"简单朴素"

朴素不是缺陷，是"确定性编译器 + 极简 token 集"的必然结果：

1. **视觉 token 极少**：3 个主题 × 每主题 8 组 token。无明暗对比设计、无强调色体系、无卡片内层级装饰；
2. **模型无权控制任何视觉**：颜色、渐变、边框、字重、间距、圆角全部由编译器决定，ALT 语法只允许 `card/theme/role` 三个属性；
3. **Button 只有一种主按钮形态**：primaryBackground + 固定渐变 + 胶囊圆角（radius=高/2），无描边/次要/危险按钮变体；
4. **Image 统一 contain + 单一 fillColor**，无圆角、无背景容器、无尺寸变化（只有角色 6 档固定值）；
5. **所有 Text 默认居中**（`textAlign=center`），稀疏树视觉上更"空旷"；
6. **间距只有 4/6/8 三档**，无 margin 布局自由度；
7. 渐变只有 root 表面和按钮两处，方向固定 `RightBottom`。

设计哲学原文（协议文档）："编译器必须为每张卡片根容器生成主题定义的多停靠渐变，并为主操作生成配套渐变。ALT 只允许模型选择主题名。" —— 这是**刻意的同质化**：小模型只需做结构决策，视觉一致性由编译器兜底。

---

## 6. 优化建议（按目标分档）

### 目标 A：让"复杂但合规"的卡片能编译通过
- 提高 `limits`：2x2 `maxVisibleTextUnits` 32→48、`maxNodes` 10→14、`maxText` 4→6；2x4 相应放宽；
- 放宽 `fontAdaptation` 下限（primary 16→12）并允许 primary 2 行（需同步改 `protectedSingleLineRoles`）；
- 给 Row 子项加 `layoutWeight` 弹性分配（先满足 min，再按比例摊剩余），替代"超限即报错"；
- 增加"溢出→降级"链：先缩字号 → 再压 gap → 再删节点，而非直接 error（扩展现有 4 次 prune 循环为分级降级）。

### 目标 B：让卡片"更丰富/更精致"
- 扩展主题体系：每主题增加 accent 强调色、次级按钮样式（描边/浅底）、卡片 header 分隔样式；
- 新增能力：允许部分场景使用 Stack（如 badge 叠加）、新增 Badge/Tag 组件、Image 支持圆角与浅色背景容器；
- 增加间距/对齐自由度（允许 ALT 声明 margin、align 白名单子集）；
- 为 Text 增加更多对齐与装饰（如 primary 数值的强调色）。

### 目标 C：保留协议纯净（推荐路线）
- 自动 ALT 专注"小卡片、可预测"，**复杂/富视觉需求走兼容路径**（`--legacy_alt` 旧几何 ALT 或直接编辑 DSL），两类能力分池，避免污染训练协议；
- 若前端嵌入（ALT 页签）落地，可同时提供"自动 ALT"与"DSL 直编"两条入口，让用户按需选择。

---

## 7. 附：关键代码位置索引

| 逻辑 | 函数 | 行号 |
| --- | --- | --- |
| 容量/协议校验 | `validate_auto_protocol` | 1487 |
| 布局校验 | `validate_layout` | 1604 |
| 文本单位/宽度 | `text_units` / `estimated_text_width` | 2501 / 2519 |
| 字号候选与拟合 | `text_font_candidates` / `fit_text_font` | 2569 / 2592 |
| 角色字号 | `auto_font` | 2697 |
| 自动布局主流程 | `auto_layout_document` | 2797 |
| 固有尺寸测量 | `measure()`（内嵌） | 2862 |
| 叶子布局赋值 | `set_leaf_layout` | 2976 |
| 空间分配 | `layout()` + `allocate_axis` + `round_axis_dimensions` | 3128 / 2682 / 2719 |
| 低优先节点删除 | `prune_auto_low_priority_leaf` | 2369 |
| 主题应用 | `apply_alt_styles` | 3395 |
| 配置 | `alt-layout-profile.json` / `alt-tuning.json` / `alt-themes.json` | — |
