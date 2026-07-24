# A2UI Card Renderer CLI v0.1.0

独立的 Harmony Card DSL 命令行渲染工具，使用与 A2UIDesigner 网页编辑器相同的 GenUI 渲染核心。

## 下载选择

- `a2ui-card-renderer-cli-v0.1.0.zip`：解压即用。Windows 可运行 `render-card.cmd`。
- `a2ui-card-renderer-cli-0.1.0.tgz`：npm 安装包，可通过 `npm install -g` 全局安装。
- `SHA256SUMS.txt`：上述两个文件的 SHA-256 校验值。

运行环境：Node.js 22 或更高版本，以及 Chrome、Edge 或 Chromium。浏览器不在标准安装位置时，通过 `A2UI_BROWSER_PATH` 指定。

## ZIP 使用方式

```powershell
render-card.cmd -i "D:\cases\card.dsl.jsonl"
render-card.cmd -i "D:\cases\card.dsl.jsonl" -o "D:\renders" -n "case-01.png"
```

也可以跨平台调用：

```powershell
node cli/render-card.js -i "D:\cases\card.dsl.jsonl"
```

## npm 安装方式

```powershell
npm install -g .\a2ui-card-renderer-cli-0.1.0.tgz
a2ui-render -i "D:\cases\card.dsl.jsonl" -o "D:\renders" -n "case-01.png"
```

## 参数

- `-i` / `--input`：输入的 `card.dsl.jsonl` 路径，必填。
- `-o` / `--output`：输出目录；默认是输入文件所在目录。
- `-n` / `--name`：输出文件名；默认是 `card.dsl.png`。
- `-h` / `--help`：显示帮助。

图片资源会依次从 DSL 相对路径、输入文件目录和工具包自带的 `references/media/` 中查找。
