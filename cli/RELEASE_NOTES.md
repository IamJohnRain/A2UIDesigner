# A2UI Card Renderer CLI v0.3.1

本版本同步 Designer 与 CLI 的 ArkUI 兼容渲染核心，修复 Progress、Divider、Button、Checkbox 和容器布局语义。

## 主要变化

- Linear Progress 使用 ArkUI 默认 4vp 轨道和圆角端点，轨道在组件外框内居中。
- Ring Progress 使用独立圆弧绘制，不再输出扇形渐变。
- Divider 的外框尺寸与实际 `strokeWidth` 分离。
- Button 使用统一内容框和原生裁剪顺序。
- Checkbox 使用 ArkUI 比例的 mark 路径。
- 容器默认 `flexShrink=0`，并支持 `layoutWeight`。
- Designer 和 CLI 继续直接使用同一份 `genui-renderer.js`。

## Linux 下载

- `a2ui-card-renderer-linux-x64-light-v0.3.1.tar.gz`：使用系统 Chrome/Chromium。
- `a2ui-card-renderer-linux-x64-full-v0.3.1.tar.gz`：内置 Google Chrome for Testing Headless Shell 151.0.7922.47 和 Noto Sans SC。
- `a2ui-card-renderer-linux-arm64-light-v0.3.1.tar.gz`：使用系统 ARM64 Chrome/Chromium。
- `a2ui-card-renderer-linux-arm64-full-v0.3.1.tar.gz`：内置 Microsoft Playwright revision 1235 的 ARM64 Headless Shell 151.0.7922.47 和 Noto Sans SC。
- `SHA256SUMS-linux.txt`：四个 Linux 包的 SHA-256。

x64 和 ARM64 完整包都会在对应架构的原生 GitHub Runner 上执行浏览器启动、真实 DSL 渲染和 PNG 分辨率检查。所有版本需要 Node.js 22+。

完整说明见 `cli/LINUX.md`，渲染兼容层的升级和回归方法见 `docs/arkui-renderer-maintenance.md`。
