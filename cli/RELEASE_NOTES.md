# A2UI Card Renderer CLI v0.3.2

本版本在 Designer 和 CLI 中统一内置未修改的 HarmonyOS Sans SC 字体，并包含 GenUI 表达式和文字裁剪修复。

## 主要变化

- 内置 HarmonyOS Sans SC Thin、Light、Regular、Medium、Bold、Black 六个原始 TTF 字重。
- Designer、CLI light/full、Linux x64/ARM64 使用完全相同的字体文件和 CSS 字重映射。
- 字体加载完成后重新执行 ArkUI paint bounds 与完整字形裁剪。
- 支持 GenUI `size(array)` 表达式。
- 保留 Noto Sans SC 作为完整离线版的缺字 fallback。

HarmonyOS Sans Fonts 版权所有 © 2021 Huawei Device Co., Ltd.。字体依据 HarmonyOS Sans Fonts License Agreement 随本软件嵌入和再分发，未进行修改，也不作为独立字体产品发布。完整协议位于 `references/fonts/LICENSE-HarmonyOS-Sans.txt`。

## Linux 下载

- `a2ui-card-renderer-linux-x64-light-v0.3.2.tar.gz`
- `a2ui-card-renderer-linux-x64-full-v0.3.2.tar.gz`
- `a2ui-card-renderer-linux-arm64-light-v0.3.2.tar.gz`
- `a2ui-card-renderer-linux-arm64-full-v0.3.2.tar.gz`
- `SHA256SUMS-linux.txt`

x64 和 ARM64 完整包会在对应架构的原生 GitHub Runner 上执行字体加载、浏览器启动、真实 DSL 渲染和 PNG 分辨率检查。所有版本需要 Node.js 22+。
