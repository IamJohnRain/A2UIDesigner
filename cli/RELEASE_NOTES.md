# A2UI Card Renderer CLI v0.3.0

本版本新增原生 Linux ARM64 发布包，并保持 Linux x64 支持。

## Linux 下载

- `a2ui-card-renderer-linux-x64-light-v0.3.0.tar.gz`：使用系统 Chrome/Chromium。
- `a2ui-card-renderer-linux-x64-full-v0.3.0.tar.gz`：内置 Google Chrome for Testing Headless Shell 151.0.7922.47 和 Noto Sans SC。
- `a2ui-card-renderer-linux-arm64-light-v0.3.0.tar.gz`：使用系统 ARM64 Chrome/Chromium。
- `a2ui-card-renderer-linux-arm64-full-v0.3.0.tar.gz`：内置 Microsoft Playwright revision 1235 的 ARM64 Headless Shell 151.0.7922.47 和 Noto Sans SC。
- `SHA256SUMS-linux.txt`：四个 Linux 包的 SHA-256。

完整包会校验包内浏览器架构，不会在 ARM64 Node.js 上误启动 x64 浏览器。x64 和 ARM64 完整包都经过原生 GitHub Runner 的真实 DSL 截图测试，140×140 Surface 输出为 490×490 PNG。

所有版本需要 Node.js 22+。完整离线版不要求系统预装浏览器和字体，但仍需要 glibc 和浏览器基础动态库；Alpine/musl 不受支持。

详细说明见 `cli/LINUX.md`。
