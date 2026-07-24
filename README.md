# Card DSL Studio

一个无需服务端、无需安装依赖的 Harmony Card DSL 可视化编辑器。

## 使用

直接双击打开 `index.html`，然后：

1. 粘贴三行 DSL JSONL，或点击“打开 JSONL”选择本地文件。
2. 点击“渲染卡片”。
3. 在画布中选择元素，使用右侧面板修改属性；可拖拽排序/换容器，拖动右下角调整尺寸。
4. 点击“保存 DSL”下载调整后的 `.jsonl` 文件。

编辑器支持 `Text`、`Image`、`Divider`、`Progress`、`Button`、`Checkbox`、`Row`、`Column`、`List` 和 `Stack`，并会解析 `updateDataModel` 中的常用 `{{ ... }}` 数据绑定。

图片协议中的 `resources/...` 属于 HarmonyOS 资源路径，普通浏览器无法直接读取，因此在编辑器里以占位图显示；路径会完整保留到导出的 DSL 中。
