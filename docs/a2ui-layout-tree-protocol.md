# ALT（A2UI Layout Tree）自动布局协议

## 1. 定位

ALT 是 A2UI Form 卡片的紧凑结构意图协议。它面向“小模型输入 TaskSpec、输出布局树”的训练与推理场景，只表达：

- 组件类型；
- 稳定节点 ID；
- 父子层级；
- 节点在信息层级中的角色；
- 卡片规格与主题选择。

模型不负责具体宽高、字号、字重、间距、圆角、颜色或溢出策略。`scripts/alt_converter.py` 根据 TaskSpec 样例文本、ALT 整体结构、GenUI 原生组件能力和版本化配置，确定性生成这些值。

ALT 的首要目标是降低布局自由度，避免文字溢出、组件重叠、不可读配色和不可控 Checkbox。它不再承担旧 DSL 的无损回转。DSL 转 ALT 后再编译得到的是满足当前自动布局规则的新 DSL，而不是原 DSL 的样式副本。

## 2. 文件职责

每个 Case 默认使用：

| 文件 | 职责 |
| --- | --- |
| `task.taskSpec.json` | 业务需求、DataModel schema、事件候选和 SVG 素材候选 |
| `card.alt.txt` | 组件结构、层级、角色、卡片规格和主题 |
| `card.asc.txt` | 按 ALT 节点补充文案、绑定、素材索引和事件索引 |
| `card.dsl.jsonl` | 编译后的三行 GenUI JSONL |
| `card.layout-report.txt` | 自动布局协议、容量和静态布局检查报告（JSON） |

ASC（A2UI Semantic Companion）不是补丁或残差。它只补充无法放进纯布局树的节点语义，不保存 DataModel、组件样式或完整事件/素材对象。

所有文本文件使用 UTF-8。ALT 与 ASC 规范化输出使用 LF；解析器同时接受 LF 和 CRLF。

## 3. 最小示例

`card.alt.txt`：

```text
Column root card=2x2 theme=neutral-light
  Row header
    Image battery_icon role=asset
    Text title role=title
  Text battery_value role=primary
  Button action_button role=action
```

`card.asc.txt`：

```text
Image battery_icon asset=0
Text title text=低电模式
Text battery_value bind=/battery/levelText
Button action_button label=立即省电 event=0
```

编译器会补齐 root 画布、安全区与背景，测量文案，为每个节点分配宽高和间距，并从主题产生文本色、按钮色、进度色与 SVG `styles.fillColor`。

## 4. ALT 语法

```text
document  ::= node newline*
node      ::= indent component id (space property)*
property  ::= "card=" card | "theme=" theme | "role=" role
indent    ::= ("  ")*
component ::= "Text" | "Image" | "Divider" | "Progress" | "Button"
            | "Checkbox" | "Row" | "Column" | "List" | "Repeat"
id        ::= [A-Za-z_][A-Za-z0-9_-]*
card      ::= "2x2" | "2x4"
theme     ::= "neutral-light" | "ambient-light" | "focus-dark"
```

规则：

- 文件直接从唯一根节点开始，不使用协议头、版本头或注释；
- 每层固定缩进两个空格，禁止 Tab 和跨层跳级；
- 节点 ID 全树唯一；
- root 只能是 `Row` 或 `Column`，且必须同时声明 `card` 和 `theme`；
- `card`、`theme` 只能出现在 root；
- 叶子节点必须声明 `role`；容器可省略 `role`；
- 自动 ALT 禁止 `Stack`。叠加属于编译器保留能力；
- 除 `card`、`theme`、`role` 外，不允许模型写任何属性。

以下写法非法：

```text
Column root card=2x2 theme=neutral-light pad=12 bg=#FFFFFFFF
  Text title role=title font=16/700 box=116x20
```

`pad`、`bg`、`font` 和 `box` 都属于编译器推断结果。

## 5. 角色

推荐角色：

```text
shell group collection item title primary status metric support meta action asset selection separator
```

角色用于排版和主题映射，不写入 DSL：

| role | 典型用途 | 编译影响 |
| --- | --- | --- |
| `title` | 卡片标题、对象名 | 中等字重，必要时最多两行 |
| `primary` | 唯一主值或主状态 | 最大视觉层级；全卡最多一个 |
| `status` / `metric` | 状态或次级指标 | 中等字号与字重 |
| `support` / `meta` | 支撑信息 | 较小字号，空间不足时优先收敛 |
| `action` | CTA | 按按钮原生内边距和文案测量固有尺寸 |
| `asset` | 语义 SVG | 按层级推断正方形尺寸和主题 `fillColor` |
| `selection` | Checkbox | 使用固定内部控件预算 |
| `separator` | Divider | 根据父布局方向推断轴向 |

同一卡片只服务一个对象或主问题，同一事实只由一个节点主承载。

## 6. 初始主题

ALT 只选择主题名，不包含具体配色。初始主题为：

| 主题 | 使用场景 |
| --- | --- |
| `neutral-light` | 默认通用浅色卡片 |
| `ambient-light` | 天气、环境、健康和设备状态等轻氛围场景 |
| `focus-dark` | 睡眠、专注、音乐或明确夜间场景 |

主题的具体 token 映射保存在 `scripts/config/alt-themes.json`。新增主题时扩展配置和协议 allowlist，不修改训练样本中的颜色。

主题负责生成：

- root 与分组表面色；
- 主、次、弱文字色；
- Button 背景与文字色；
- Progress、Checkbox 和 Divider 颜色；
- SVG 的 `Image.styles.fillColor`。

编译器输出 DSL hex，不把主题名写入 DSL。

## 7. SVG 素材

自动 ALT 只允许本地 SVG：

- ALT 只声明 `Image ... role=asset`，不写路径、尺寸和颜色；
- ASC 只用 `asset=N` 引用 TaskSpec `assetCandidates`；
- 被引用候选的 `src` 必须以 `.svg` 结尾；
- 禁止 PNG、网络 URL、data URL、base64、emoji 和模型直接输出 `src`；
- 编译器固定生成 `objectFit: "contain"`，并按角色推断宽高；
- 编译器从主题生成静态 `styles.fillColor`，模型禁止指定素材颜色。

GenUI Image 新增的 `styles.fillColor` 只接受静态 `#RRGGBB` 或 `#AARRGGBB`。它不改变 Image 原有 `aspectRatio=1`、`objectFit=cover` 等默认参数；自动编译器只在生成的 DSL 中显式覆盖需要的值。

## 8. 自动布局推断

推断配置位于 `scripts/config/alt-layout-profile.json`，当前 profile 对齐 `genui@0.7.0-alpha.7`。

### 8.1 画布和安全区

| card | 画布 | root padding | root radius |
| --- | --- | --- | --- |
| `2x2` | `140 x 140vp` | `12vp` | `18vp` |
| `2x4` | `300 x 140vp` | `12vp` | `22vp` |

root 固定裁切，背景由主题提供。Row 横向分配空间，Column 纵向分配空间；所有子项、gap 和 root padding 都进入预算。

### 8.2 文本

编译器用 Unicode East Asian Width 和保守的英文、数字、空格权重计算“中文等价单位”，再结合角色选择字号、字重和最大行数。标题、主值、状态和 CTA 是受保护文本，不通过裁剪、省略或覆盖解决溢出。

动态绑定优先使用 TaskSpec `sampleValue` 测量。复杂表达式无法静态求值时使用语义回退并在布局报告中给出 warning。

### 8.3 Button

GenUI Button 核心默认参数保持不变：

- 默认垂直内边距 `8vp`；
- 默认水平内边距 `12vp`；
- 默认字号 `16fp`；
- 默认字重 `500`。

自动布局器会根据动作角色、整体密度和实际标签选择允许的字号/字重，然后按“测量文本宽度 + 原生水平内边距 + 安全余量”推断按钮宽度。高度至少覆盖文字行高与原生垂直内边距，主 CTA 不低于 profile 最小高度。任何无法容纳完整标签的按钮都会使自动编译失败，不输出明知会遮挡文字的 DSL。

### 8.4 Checkbox

Checkbox 不能通过 DSL 可靠缩放内部控件。profile 按以下固定能力预算：

- 外层固有高度 `48vp`；
- 内部控件 `20 x 20vp`；
- 控件 margin `2vp`；
- 标签前固定 gap `12vp`；
- 内部标签字号约 `16fp`，单行。

因此：

- `2x2` 禁止 Checkbox；
- `2x4` 最多一个；
- 只有明确的勾选、选择或布尔设置才使用；
- 只展示状态使用 Text，表达动作使用 Button；
- 无法满足 `48vp` 高度和完整标签宽度时编译失败，不缩小内部控件。

### 8.5 其他组件

- Image：按 `role` 推断 `18-56vp` 正方形尺寸；
- Progress：主进度使用 ring，支撑进度使用 linear，并推断宽高；
- Divider：Row 子项推断为竖线，其余推断为横线；
- List：仅 `2x4` 可用，最多一个，最多规划三个可见项；
- Repeat：只用于 List/Row/Column 的单一重复模板，集合路径进入 ASC/TaskSpec，不进入 ALT。

## 9. 结构与内容容量

| 约束 | `2x2` | `2x4` |
| --- | ---: | ---: |
| 最大层级 | 4 | 5 |
| 最大节点数 | 10 | 18 |
| Text | 4 | 8 |
| Button | 1 | 2 |
| Image | 2 | 3 |
| Progress | 1 | 2 |
| Checkbox | 0 | 1 |
| List | 0 | 1 |
| 可见文本预算 | 32 单位 | 72 单位 |

普通 Row/Column 最多三个直接子节点，禁止空容器和未挂载节点。放不下时应在模型输出阶段依次删除 meta、次要 asset、弱 support、第二动作和重复事实，而不是输出更多样式参数。

## 10. ASC 语法

ASC 每行使用 `Component node_id key=value`，只输出存在语义补充的节点，并严格遵循 ALT 前序顺序。

| 组件 | 允许字段 |
| --- | --- |
| Text | `text`、`bind`、`expr` |
| Image | `asset` |
| Progress | `value`、`total` |
| Button | `label`、`event` |
| Checkbox | `label`、`value`、`group`、`bind`、`select`、`event` |

约束：

- `bind`、`value`、`total` 使用 JSON Pointer；复杂绑定才用完整 `expr`；
- `asset=N` 和 `event=N` 只引用 TaskSpec 候选索引；
- ASC 不复制素材路径、完整事件、DataModel、样式、尺寸或颜色；
- ASC 节点必须与 ALT 中同 ID 节点的组件类型一致；
- 无补充信息的容器不写入 ASC。

DataModel 始终由 TaskSpec `dataModelSchema.sampleValue` 递归生成。

## 11. 编译与失败边界

`TaskSpec + ALT + ASC -> DSL` 流程：

1. 校验 ALT 语法、主题、卡片规格、层级和组件数量；
2. 校验 ASC 节点映射及 SVG/事件索引；
3. 从 TaskSpec 生成 sample DataModel 并解析可测量文案；
4. 按 profile 测量叶子组件固有尺寸；
5. 自顶向下分配 Row/Column 空间；
6. 应用主题，生成具体样式与 SVG `fillColor`；
7. 执行静态布局检查并写布局报告；
8. 只有不存在 hard error 时才覆盖目标 DSL。

自动 ALT 可以被拒绝。以下情况属于 hard error：

- 超出节点、层级、组件或文本容量；
- TaskSpec 卡片规格与 ALT `card` 不一致；
- Image 未解析到本地 SVG；
- Button、Checkbox 或受保护文本无法完整显示；
- Row/Column 最小尺寸超过可用空间；
- 违反 Checkbox、List、Stack 等组件边界。

编译器不会通过裁剪、遮挡、缩小 Checkbox 内部控件或修改 GenUI 核心默认参数来强行生成。

## 12. 兼容与非等价声明

转换器仍能读取旧几何 ALT，便于现有数据迁移；`d2t --legacy_alt` 也可临时输出旧格式。但该格式仅作为兼容入口，不是新的训练协议。

默认 `d2t` 会：

- 保留可达组件树、稳定 ID 和角色；
- 抽取内容/绑定/事件/素材为 ASC；
- 丢弃旧 DSL 的具体尺寸、字号、间距、颜色和其他可推断样式；
- 根据旧 root 背景近似选择三个初始主题之一。

所以默认 `DSL -> ALT/ASC -> DSL` 只要求结构和业务语义可迁移，不要求旧样式或 JSON 规范化等价。需要审计旧布局时使用 `--legacy_alt` 和原 DSL，不把旧几何样本混入自动 ALT 训练集。

## 13. 批量转换工具

DSL 转自动 ALT/ASC：

```powershell
python scripts/alt_converter.py `
  -o references/datasets `
  --mode d2t
```

自动 ALT/ASC 编译为新 DSL：

```powershell
python scripts/alt_converter.py `
  -o references/datasets `
  --mode t2d `
  --dsl_name card.auto.dsl.jsonl
```

旧几何 ALT 兼容导出：

```powershell
python scripts/alt_converter.py `
  -o references/datasets `
  --mode d2t `
  --legacy_alt `
  --alt_name card.legacy.alt.txt
```

| 参数 | 必需 | 默认值 | 含义 |
| --- | --- | --- | --- |
| `-o DATASET_DIR` | 是 | 无 | 每个直接子目录为一个 Case |
| `--mode d2t/t2d` | 是 | 无 | 转换方向 |
| `--dsl_name` | 否 | `card.dsl.jsonl` | Case 内 DSL 文件名 |
| `--alt_name` | 否 | `card.alt.txt` | Case 内 ALT 文件名 |
| `--asc_name` | 否 | `card.asc.txt` | Case 内 ASC 文件名 |
| `--taskspec_name` | 否 | `task.taskSpec.json` | Case 内 TaskSpec 文件名 |
| `--layout_report_name` | 否 | `card.layout-report.txt` | t2d 布局报告名 |
| `--legacy_alt` | 否 | 关闭 | 仅 d2t：输出旧几何 ALT |

脚本逐 Case 打印标准化进度。单个 Case 失败不会中断其他 Case；存在失败 Case 时进程返回非零。自动布局有 hard error 时仍会写布局报告，但不会覆盖目标 DSL。

## 14. 训练 RequestBody

`scripts/taskspec_to_alt_chat_completions.py` 将 TaskSpec 压缩为 PlanningSpec，并把 `card.alt.txt`、`card.asc.txt` 组装为 Assistant 内容。

PlanningSpec 只向模型暴露：

- userQuery 与卡片规格；
- schema 字段路径、类型、样例和文本单位；
- 事件候选索引；
- SVG 素材候选索引和描述；
- 当前规格的结构与文本容量；
- 三个可选主题名。

Assistant 固定格式：

```text
<alt>
Column root card=2x2 theme=neutral-light
  Text title role=title
  Text primary_value role=primary
</alt>
<asc>
Text title text=今日用电
Text primary_value bind=/power/value
</asc>
```

提示词禁止模型输出具体样式、尺寸、字体、颜色、素材路径和 DataModel，并要求在容量不足时删减信息，而不是尝试手工布局。
