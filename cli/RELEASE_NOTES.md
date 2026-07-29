# A2UI Card Renderer CLI v0.2.0

独立的 Harmony Card DSL 命令行渲染工具，使用与 A2UIDesigner 网页编辑器相同的 GenUI 渲染核心。

## Linux 下载选择

- `a2ui-card-renderer-linux-x64-light-v0.2.0.tar.gz`：轻量版，使用系统 Chrome/Chromium。
- `a2ui-card-renderer-linux-x64-full-v0.2.0.tar.gz`：完整离线版，内置固定版本的 chrome-headless-shell 和 Noto Sans SC 字体。
- `SHA256SUMS-linux.txt`：两个 Linux 包的 SHA-256 校验值。

两个版本均需要 Node.js 22 或更高版本。轻量版需要系统安装 Chrome/Chromium；完整离线版不需要系统浏览器和字体，但仍需要 glibc 及 chrome-headless-shell 的基础动态库。

完整包固定使用 Chrome for Testing 151.0.7922.47 和 Noto Sans CJK 2.004。CLI 优先使用 `A2UI_BROWSER_PATH`，其次使用包内运行时，最后回退到系统浏览器。

## Linux 使用方式

```bash
tar -xzf a2ui-card-renderer-linux-x64-full-v0.2.0.tar.gz
cd a2ui-card-renderer
./render-card -i ./card.dsl.jsonl
./render-card -i ./card.dsl.jsonl -o ./renders -n case-01.png
```

也可以直接调用 Node.js 入口：

```bash
node cli/render-card.js -i ./card.dsl.jsonl
```

## 参数

- `-i` / `--input`：输入的 `card.dsl.jsonl` 路径，必填。
- `-o` / `--output`：输出目录；默认是输入文件所在目录。
- `-n` / `--name`：输出文件名；默认是 `card.dsl.png`。
- `-h` / `--help`：显示帮助。

图片资源会依次从 DSL 相对路径、输入文件目录和工具包自带的 `references/media/` 中查找。
