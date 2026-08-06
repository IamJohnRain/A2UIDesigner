# Card DSL Studio

> 本软件内置并使用 HarmonyOS Sans SC 字体。字体版权归 Huawei Device Co., Ltd. 所有，随软件打包使用和再分发遵循 `references/fonts/LICENSE-HarmonyOS-Sans.txt`；字体不得作为独立字体产品分发，也未对字体文件进行修改。

一个无需服务端、无需安装依赖的 Harmony Card DSL 可视化编辑器。

## 使用

直接打开 `index.html`，然后：

1. 粘贴三行 DSL JSONL，或点击“打开 JSONL”选择本地文件。
2. 点击“渲染卡片”。
3. 在画布中选择元素并使用右侧面板修改属性，也可以拖拽排序、切换容器和调整尺寸。
4. 点击“保存 DSL”下载修改后的 `.jsonl` 文件。

编辑器支持 `Text`、`Image`、`Divider`、`Progress`、`Button`、`Checkbox`、`Row`、`Column`、`List` 和 `Stack`，并解析 `updateDataModel` 中常用的 `{{ ... }}` 数据绑定。

协议资源路径 `resources/base/media/...` 会在网页预览时映射到仓库的 `references/media/...`，导出的 DSL 仍保留协议路径。

## 在线访问

https://iamjohnrain.github.io/A2UIDesigner/

## ALT 生成（浏览器内转换）

「ALT 生成」页签在浏览器本地完成 TaskSpec + ALT + ASC → DSL 转换，转换内核为 `scripts/alt_to_dsl_converter.py`，由 Pyodide 在浏览器内执行。运行时（core 0.26.4，含时区补丁）首次点击「编译并渲染」时从 `https://iamjohnrain.github.io/a2ui-pyodide/` 懒加载，主源不可用时自动回退到 `raw.githubusercontent.com/IamJohnRain/a2ui-pyodide/master/`。详见 `docs/alt-protocol-pyodide-plan.md`。

## CLI 渲染器

[GitHub Release](https://github.com/IamJohnRain/A2UIDesigner/releases/latest) 提供 Linux x64 和 ARM64 的两类版本：

- 轻量版：使用系统已安装的 Chrome/Chromium。
- 完整离线版：内置固定版本的 headless shell 和 Noto Sans SC 字体，不要求系统预装浏览器或字体。

所有版本需要 Node.js 22+。解压后运行：

```bash
./render-card -i ./card.dsl.jsonl
```

完整参数、架构选择、浏览器来源和系统依赖见 [`cli/LINUX.md`](cli/LINUX.md)。
