# A2UI Card Renderer CLI

使用与网页编辑器相同的 GenUI 渲染核心，将 `card.dsl.jsonl` 输出为 PNG。包本身无第三方依赖，需要 Node.js 22 或更高版本，以及本机安装的 Chrome、Edge 或 Chromium。

PNG 使用与原生渲染结果一致的 `3.5x` 设备像素密度：`300x140` Surface 输出 `1050x490` PNG，`140x140` Surface 输出 `490x490` PNG。高分辨率由浏览器直接栅格化，不是对低分辨率截图进行拉伸。

```powershell
node cli/render-card.js -i "D:\cases\card.dsl.jsonl"
node cli/render-card.js -i "D:\cases\card.dsl.jsonl" -o "D:\renders"
node cli/render-card.js -i "D:\cases\card.dsl.jsonl" -o "D:\renders" -n "case-01.png"
```

Release ZIP 在 Windows 下也可以直接使用根目录启动器：

```powershell
render-card.cmd -i "D:\cases\card.dsl.jsonl" -o "D:\renders" -n "case-01.png"
```

也可以在仓库中执行一次 `npm link`，之后直接使用：

```powershell
a2ui-render -i "D:\cases\card.dsl.jsonl" -o "D:\renders" -n "case-01.png"
```

执行 `npm pack` 会生成可复制到其他环境安装的独立 `.tgz` 包，包内包含 CLI 和 GenUI 浏览器渲染核心，不需要把编辑器其他文件一起打包。

图片资源按以下顺序查找：输入文件目录下的 DSL 相对路径、输入文件目录下的同名文件、包内的 `references/media/`。因此既能使用与 DSL 一起分发的资源，也能继续解析编辑器已有的 `resources/base/media/...` 预览资源。

参数：

- `-i` / `--input`：输入文件路径，必填。
- `-o` / `--output`：输出目录，可选；默认是输入文件所在目录。
- `-n` / `--name`：输出文件名，可选；默认是 `card.dsl.png`。
- `-h` / `--help`：显示命令帮助。

如果浏览器安装在非标准位置，请设置 `A2UI_BROWSER_PATH`：

```powershell
$env:A2UI_BROWSER_PATH = "D:\Apps\Chrome\chrome.exe"
```
