# Linux packages

Release 同时提供 Linux x64 和 ARM64（AArch64）版本。所有包都需要 Node.js 22 或更高版本。

## 下载选择

| 架构 | 轻量版 | 完整离线版 |
|---|---|---|
| x64 | `a2ui-card-renderer-linux-x64-light-v0.3.1.tar.gz` | `a2ui-card-renderer-linux-x64-full-v0.3.1.tar.gz` |
| ARM64 | `a2ui-card-renderer-linux-arm64-light-v0.3.1.tar.gz` | `a2ui-card-renderer-linux-arm64-full-v0.3.1.tar.gz` |

轻量版使用系统 Chrome/Chromium。完整离线版内置与 CPU 架构匹配的 headless shell、Noto Sans SC 字体和私有 Fontconfig 配置，不需要另外安装浏览器或字体。

## 浏览器来源

- x64：Google Chrome for Testing `chrome-headless-shell` 151.0.7922.47。
- ARM64：Microsoft Playwright CDN revision 1235，Chromium Headless Shell 151.0.7922.47。它是 Playwright 的 Linux ARM64 non-CfT 构建，不是 Google 发布的 Linux ARM64 CfT 二进制。
- 字体：Noto Sans CJK 2.004 中的 Noto Sans SC。

下载源、版本和 SHA-256 都固定在 `tools/build-linux-packages.sh`。Release 中的 `SHA256SUMS-linux.txt` 用于校验最终压缩包。

## 使用

```bash
tar -xzf a2ui-card-renderer-linux-arm64-full-v0.3.1.tar.gz
cd a2ui-card-renderer
./render-card -i ./card.dsl.jsonl
./render-card -i ./card.dsl.jsonl -o ./renders -n case-01.png
```

浏览器选择顺序：

1. `A2UI_BROWSER_PATH` 指定的浏览器。
2. 与当前 `process.arch` 匹配的包内浏览器。
3. 标准系统路径中的 Chrome/Chromium。

完整包中的 `runtime/ARCHITECTURE` 用于防止 ARM64 Node.js 误启动 x64 浏览器，或反向误用。需要覆盖浏览器时：

```bash
A2UI_BROWSER_PATH=/opt/chromium/chrome ./render-card -i ./card.dsl.jsonl
```

设置 `A2UI_DEBUG=1` 可以输出浏览器选择、平台架构和渲染阶段信息。

## 系统兼容性

完整包仍依赖 glibc 和浏览器所需的基础动态库。ARM64 构建面向 Ubuntu 20.04/22.04/24.04/26.04 ARM64 和 Debian 12/13 ARM64；其他 glibc 发行版需要自行验证。Alpine/musl 不受支持。

便携 headless shell 不包含可安装的 setuid sandbox，因此 CLI 只在启动包内浏览器时添加 `--no-sandbox`。通过 `A2UI_BROWSER_PATH` 指定的浏览器及系统浏览器保留其正常沙箱设置。在共享主机上使用完整包时，只渲染可信 DSL 和资源。

## 发布验证

Release 工作流会在原生 GitHub x64 和 ARM64 Runner 上分别执行：

- 可执行文件架构检查。
- 浏览器版本启动检查。
- 真实 DSL 渲染。
- PNG 分辨率检查（140×140 Surface 输出 490×490 PNG）。

只有两个架构都通过后才会创建或更新 Release。
