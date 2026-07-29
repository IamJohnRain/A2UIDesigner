# Card DSL Studio

一个无需服务端、无需安装依赖的 Harmony Card DSL 可视化编辑器。

## 使用

直接双击打开 `index.html`，然后：

1. 粘贴三行 DSL JSONL，或点击“打开 JSONL”选择本地文件。
2. 点击“渲染卡片”。
3. 在画布中选择元素，使用右侧面板修改属性；可拖拽排序/换容器，拖动右下角调整尺寸。
4. 点击“保存 DSL”下载调整后的 `.jsonl` 文件。

编辑器支持 `Text`、`Image`、`Divider`、`Progress`、`Button`、`Checkbox`、`Row`、`Column`、`List` 和 `Stack`，并会解析 `updateDataModel` 中的常用 `{{ ... }}` 数据绑定。

图片协议中的 `resources/base/media/...` 会在网页预览时自动映射到仓库的 `references/media/...`。导出时会反向规范化为协议路径，不会把 GitHub Pages 的预览地址写入 DSL。

## 在线访问

GitHub Pages 启用后可通过以下地址使用：

https://iamjohnrain.github.io/A2UIDesigner/

## CLI 渲染器

Release 提供 Linux x64 的两个版本：

- 轻量版：使用系统已安装的 Chrome/Chromium。
- 完整离线版：内置固定版本的 `chrome-headless-shell` 和 Noto Sans SC 字体，不要求系统预装浏览器或字体。

两个版本均需要 Node.js 22+。解压后运行：

```bash
./render-card -i ./card.dsl.jsonl
```

完整参数、系统依赖和浏览器覆盖方式见 [`cli/LINUX.md`](cli/LINUX.md)。
