# ALT 布局超参配置（`scripts/config/alt-tuning.json`）

本文档说明 `scripts/config/alt-tuning.json` 中每个参数的含义。该文件集中存放
`scripts/alt_converter.py` 自动布局（ALT → DSL）曾经硬编码在代码里的可调常数，
包括文本测量安全系数、组件几何尺寸、密度切换阈值、列布局阈值、数值容差与主题推断阈值。

代码通过 `tuning_value("section.key", default)` 读取参数；`default` 与旧硬编码值一致，
因此删除某个 key 不会静默改变行为，只会回退到默认值。布局结构规则（canvas / limits /
spacing / textRules / typography）仍在 `alt-layout-profile.json`，主题色 token 仍在
`alt-themes.json`。

## text —— 文本测量

| 参数 | 默认 | 含义 |
| --- | --- | --- |
| `text.widthSafety` | 1.10 | 文本宽度安全系数：`预估宽度 = 字符单位 × 字号 × 字重系数 × safety`。>1 会给文字留出渲染余量，防止真实渲染时截断。 |
| `text.buttonWidthSafety` | 1.08 | Button label 的宽度安全系数，比普通文本略紧，因为按钮文案通常更短。 |
| `text.lineHeightPadding` | 4.0 | 行高纵向余量：`行高 = 字号 + lineHeightPadding`（vp），用于 `maxLines` 高度预算。 |
| `text.minimumWidthChars` | 2.0 | 文本最小宽度 = 最小可用字号 × 该值（约等于“至少两个字”的宽度下限）。 |
| `text.longTextUnitsThreshold` | 6.0 | 长文本判定阈值：`maxLines > 1` 只在内容单位数超过该值时才生效；短文本强制单行。 |
| `text.weightFactorDivisor` | 5000.0 | 字重系数分母：`1 + max(0, 字重 − 400) / divisor`。决定加粗对预估宽度的放大程度。 |
| `text.unitWeights.space` | 0.35 | 空格字符的宽度单位。 |
| `text.unitWeights.cjk` | 1.0 | 全角/中日韩字符（East Asian W/F）的宽度单位。 |
| `text.unitWeights.upper` | 0.68 | 大写字母的宽度单位。 |
| `text.unitWeights.lower` | 0.56 | 小写字母的宽度单位。 |
| `text.unitWeights.digit` | 0.62 | 数字的宽度单位。 |
| `text.unitWeights.other` | 0.45 | 其他字符的宽度单位。 |

这些单位权重是 `estimated_text_width()` / `text_units()` 的测量基础，改动会直接影响所有
文本、Button、Checkbox 的预估宽度与溢出判定。

## button —— 按钮测量

| 参数 | 默认 | 含义 |
| --- | --- | --- |
| `button.charsHorizontalReserve` | 24.0 | 按钮横向预留（vp）：计算 `chars` 容量（`floor((宽 − reserve) / 字号)`）和 label 宽度校验（`文本宽 + reserve`）时扣除的固定内边距。 |
| `button.labelFontFallback` | 16.0 | 兼容路径下 Button 缺少 `font` 属性时，测量 label 使用的回退字号。 |

## components —— 组件几何尺寸

> 该区块原位于 `alt-layout-profile.json`，现统一归入本文件。

### `components.button` / `components.autoButton`

| 参数 | button | autoButton | 含义 |
| --- | --- | --- | --- |
| `paddingVertical` | 8 | 7 | 按钮上下内边距（vp），参与固有高度计算。 |
| `paddingHorizontal` | 12 | 12 | 按钮左右内边距（vp），参与固有宽度计算。 |
| `minimumHeight` | 32 | 32 | 按钮最小可点击高度（vp）。 |
| `minimumWidth` | 48 | 48 | 按钮最小宽度（vp）。 |
| `widthSafety` | 4 | 4 | 按钮宽度额外安全余量（vp），叠加在文本+内边距之上。 |

### `components.checkbox` / `components.autoCheckbox`

| 参数 | checkbox | autoCheckbox | 含义 |
| --- | --- | --- | --- |
| `outerHeight` | 48 | 22 | 勾选项整体高度（vp）。自动布局使用 22（紧凑），兼容路径使用 48。 |
| `controlSize` | 20 | 20 | 勾选控件（方框）边长（vp）。 |
| `controlMargin` | 2 | 2 | 控件四周外边距（vp）。 |
| `labelGap` | 12 | 12 | 控件与 label 之间的间距（vp）。 |
| `labelFontSize` | 16 | 16 | label 字号（fp），用于测量 label 宽度。 |
| `minimumWidth` | — | 36 | 勾选项最小宽度（vp），等于固定部分 `controlSize + 2×controlMargin + labelGap`。 |

勾选项固有宽度 = `controlSize + 2×controlMargin + labelGap + 文本预估宽`。该宽度是
“内容宽度”，不会拉伸填满父容器，因此这些参数直接决定渲染时勾选项的宽度与两侧留白。

### `components.image`

| 参数 | 默认 | 含义 |
| --- | --- | --- |
| `meta` | 18 | 次要信息图标边长（vp）。 |
| `support` | 20 | 辅助图标边长（vp）。 |
| `action` | 20 | 动作图标边长（vp）。 |
| `asset` | 24 | 普通素材图标边长（vp）。 |
| `primary2x2` | 48 | 2x2 卡主图/大图标边长（vp）。 |
| `primary2x4` | 56 | 2x4 卡主图/大图标边长（vp）。 |

### `components.progress`

| 参数 | 默认 | 含义 |
| --- | --- | --- |
| `ring2x2` | 56 | 2x2 环形进度边长（vp）。 |
| `ring2x4` | 64 | 2x4 环形进度边长（vp）。 |
| `linearHeight` | 8 | 线性进度条高度（vp）。 |
| `linearMinimumWidth` | 64 | 线性进度条最小宽度（vp）。 |

### `components.divider`

| 参数 | 默认 | 含义 |
| --- | --- | --- |
| `stroke` | "1px" | 分隔线描边宽度。 |

## density —— 紧凑密度切换

| 参数 | 默认 | 含义 |
| --- | --- | --- |
| `density.compactNodeThreshold2x2` | 7 | 2x2 卡节点数超过该值即切到 compact 密度。 |
| `density.compactNodeThreshold2x4` | 13 | 2x4 卡节点数超过该值即切到 compact 密度。 |
| `density.compactUnitsRatio` | 0.72 | 可见文本单位超过 `maxVisibleTextUnits × ratio` 即切到 compact 密度。 |

compact 密度会使用 `alt-layout-profile.json` 的 `typography.compact` 字号档位，并对
3 个及以上子节点的容器使用 `denseGap`。

## column —— Column 布局阈值

| 参数 | 默认 | 含义 |
| --- | --- | --- |
| `column.centerMainRatio` | 0.7 | 子节点高度总和 < 容器内容高 × 该值（且非底置按钮场景）时，子节点垂直居中；否则 `justifyContent: start`。 |
| `column.bottomActionAnchorGap` | 18.0 | 以 Button 结尾且子节点高度总和距容器底部空余 > 该值（vp）时，改用 `spaceBetween` 把按钮锚定到底部。 |
| `column.compactGapMinimum` | 2.0 | Row/Column 最小预算放不下时，允许把 itemMargin 压缩到的下限；压缩步长与下限均为该值。 |

## tolerance —— 数值容差

| 参数 | 默认 | 含义 |
| --- | --- | --- |
| `tolerance.comparisonEpsilon` | 0.01 | 所有溢出/放不下判定使用的浮点容差（vp），避免 1e-16 级浮点误差误报。 |

## typography —— 角色字号自适应

| 参数 | 默认 | 含义 |
| --- | --- | --- |
| `typography.primaryFontBands` | `[{"aboveUnits":12,"fontSize":16},{"aboveUnits":8,"fontSize":18},{"aboveUnits":3,"fontSize":20}]` | `primary` 角色按内容长度降档：单位数 > 12 用 16fp，> 8 用 18fp，> 3 用 20fp。数组顺序必须按 `aboveUnits` 从大到小，命中第一档即停止。 |

## fontAdaptation —— 字号适配回退默认值

当 `alt-layout-profile.json` 的 `textRules.fontAdaptation` 未覆盖某角色时使用：

| 参数 | 默认 | 含义 |
| --- | --- | --- |
| `fontAdaptation.defaultMinSize` | 10 | 回退最小字号（fp），与 `preferred − 4` 取较大者。 |
| `fontAdaptation.defaultStep` | 2 | 回退降档步长（fp）。 |
| `fontAdaptation.absoluteMinimumSize` | 8 | 字号绝对下限（fp），任何角色不得低于该值。 |

## themeInference —— 兼容 ALT 主题推断

仅用于旧版几何 ALT（带 `bg` 属性）自动推断主题：

| 参数 | 默认 | 含义 |
| --- | --- | --- |
| `themeInference.luminanceWeights` | [0.2126, 0.7152, 0.0722] | 计算背景相对亮度的 RGB 权重（Rec.601）。 |
| `themeInference.darkLuminanceThreshold` | 0.42 | 亮度低于该值判定为深色 → `focus-dark`。 |
| `themeInference.ambientChannelSpread` | 12.0 | RGB 通道极差 ≥ 该值判定为彩色 → `ambient-light`。 |
| `themeInference.ambientMinChannel` | 238.0 | RGB 最小通道 < 该值判定为非近白 → `ambient-light`。 |

## 有意留在代码里的常数

以下数值不属于可调超参，未抽取：

- 取整 epsilon `1e-9`（`rounded_dimension` / `round_axis_dimensions` 的浮点取整保护）；
- `max(1, ...)` 之类的尺寸下限钳制；
- `primary` 的“时间/时长文案仍用 20fp”语义规则；
- `alt-layout-profile.json` 的 canvas / limits / spacing / textRules / typography 结构规则；
- `alt-themes.json` 的主题色 token。

## 同步注意

`cli/render-card.js` 使用的 `genui-renderer.js` 内也内嵌了 Checkbox 的渲染默认值
（controlSize 20 / controlMargin 2 / labelSpacing 12 / labelFontSize 16）与按钮默认值。
修改 `components.*` 时，请同步核对渲染器默认值，避免 DSL 与预览不一致。
