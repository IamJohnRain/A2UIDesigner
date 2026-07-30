# ArkUI 兼容渲染器维护指南

## 目标与唯一实现

本项目以 `genui-renderer.js` 作为唯一的 GenUI/ArkUI 兼容渲染核心。Designer 页面和 CLI 不得各自实现组件默认值、布局或绘制规则：

- Designer 由 `index.html` 直接加载 `genui-renderer.js`；
- CLI 的 `cli/renderer.html` 同样加载根目录的 `genui-renderer.js`；
- CLI 打包脚本必须把该文件原样放入发布包，不维护副本；
- `app.js` 只负责编辑器交互，`cli/browser-renderer.js` 只负责 DSL 解析、节点构造和资源等待。

任何视觉兼容修复都应进入共享核心，编辑器选框、拖拽手柄、缺失资源诊断等辅助效果则留在编辑器层。

## 兼容模型

渲染遵循 ArkUI 的三个阶段：

1. Measure：解析 GenUI 默认值、DSL 尺寸和父容器约束；
2. Layout：区分节点 frame、padding/content box 和内部 paint bounds；
3. Paint：在 paint bounds 内绘制轨道、笔画、标记或文字。

不得把 DSL 的 `width`/`height` 直接当成内部笔画尺寸。例如：

- linear Progress 的节点可以高于 4vp，但默认轨道仍为 4vp并垂直居中；
- Divider 的实际厚度为 `min(strokeWidth, crossAxisConstraint)`；
- Button 的显式尺寸约束外框，padding 仍参与 label 内容区和裁剪；
- Checkbox 的 48vp 外框不等于内部 20×20 控件尺寸。

共享核心顶部的 `compatibilityProfile` 是当前版本的默认参数入口。默认值必须能追溯到 `references/genui/` 或 `references/arkui_ace_engine/`，禁止为单个 Case 写 ID、路径或像素特例。

## 当前关键原生映射

| 组件 | 兼容规则 | 原生依据 |
| --- | --- | --- |
| Button | padding 8/12vp、fontSize 16fp、fontWeight 500；由单一内容框居中并在外框裁剪 | `ExtendedButtonComponent.cpp` |
| Progress linear/ring | 默认 strokeWidth 4vp、radius 为 strokeWidth/2；linear 轨道在节点内居中，ring 使用独立圆弧而非扇形渐变 | `progress_option.cpp`、`progress_layout_algorithm.cpp`、`progress_paint_method.h` |
| Divider | 默认 1vp，笔画与节点约束分离；支持 lineCap | `divider_layout_algorithm.cpp`、`divider_paint_method.h` |
| Checkbox | 外框 48vp、控件 20vp、margin 2vp、label 间距 12vp；mark 使用 ArkUI 比例路径 | `ExtendedCheckboxComponent.cpp`、`checkbox_paint_method.cpp` |
| 容器子项 | 默认不收缩，只有显式 `flexShrink` 才改变 | `ExtendedStyleResolver.cpp` |
| Image | 默认 aspectRatio 1、objectFit cover、原生 placeholder alt | `ExtendedImageComponent.cpp` |

## 升级 GenUI 或 ArkUI 源码

收到新源码包时按以下顺序维护：

1. 替换 `references/genui/`，同时记录它对应的 ArkUI/API 版本；
2. 更新或替换 `references/arkui_ace_engine/`；
3. 对比 GenUI Extended 组件中的 `DEFAULT_*`、构造函数和 `SetNode*` 调用；
4. 继续追踪 ArkUI 对应组件的 theme、layout algorithm、paint method/modifier；
5. 只修改 `compatibilityProfile` 和共享 measure/layout/paint primitive；
6. 同时用 Designer 和 CLI 渲染同一份 DSL，确认 DOM 几何与 PNG 一致；
7. 完成组件 fixture 和数据集 PNG 回归后再发布 CLI。

重点检查的变化包括默认 padding/字号、theme 色值、strokeWidth/radius、lineCap、字体缩放、disabled alpha、flexShrink、layoutWeight、clip 和 objectFit 枚举。

## 回归矩阵

### 字体基线

Designer 和 CLI 使用 `references/fonts/` 中未修改的 HarmonyOS Sans SC TTF。任何打包方式都必须保留六个字重及 `LICENSE-HarmonyOS-Sans.txt`，不得对子集化、转换格式或修改字体。渲染截图前必须等待 `document.fonts.ready`；Noto Sans SC 和系统字体只作为缺字 fallback。

每次改变共享渲染器至少检查：

- 140×140 与 300×140 卡片；
- CLI 原生输出倍率以及 DPR 1、2、3.5；
- light/dark（涉及主题值时）；
- 正常尺寸、恰好容纳和空间不足三类约束；
- Progress 的 0%、小值、50%、100% 以及所有 type；
- 横向/纵向 Divider，不同外框和 strokeWidth；
- Button 正常显示和上下文字裁剪；
- Text 单行、多行、clip、ellipsis 和中英文混排；
- Checkbox 选中、未选中、circle、square；
- Row/Column/List 的固定尺寸、layoutWeight 和 flexShrink。

除了整图 pixel diff，还应核对节点 frame bbox、内部 paint bbox、文字基线/行框、主色区域和 alpha 边缘。字体回归必须使用 CLI 发布包内固定字体或明确记录系统字体版本。

## 发布前检查

```powershell
node --check genui-renderer.js
node --check app.js
node --check cli/browser-renderer.js
node --check cli/render-card.js
git diff --check
```

随后用同一 DSL 分别在 Designer 和 CLI 渲染。两者应报告相同的 `GenUIRenderer.compatibilityProfile.id`，组件 DOM 的 frame/paint 尺寸也应一致。

禁止以下做法：

- 在 CLI 或 Designer 中复制一份 renderer；
- 根据 Case 名称、目录或截图写条件分支；
- 修改 DSL 来掩盖原生默认值差异；
- 用全局 CSS 圆角替代 ArkUI strokeRadius/lineCap；
- 未完成原生源码追踪和回归就调整核心默认 padding、字号或控件尺寸。
