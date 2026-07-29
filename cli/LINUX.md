# Linux packages

The release provides two Linux x64 archives.

## Lightweight package

`a2ui-card-renderer-linux-x64-light-v0.2.0.tar.gz` contains the CLI, renderer and preview assets. It uses, in order:

1. `A2UI_BROWSER_PATH`.
2. A bundled runtime when present.
3. Chrome or Chromium installed in a standard system path.

Install Node.js 22+ and Chrome/Chromium, then run:

```bash
./render-card -i ./card.dsl.jsonl
```

## Full offline package

`a2ui-card-renderer-linux-x64-full-v0.2.0.tar.gz` additionally contains:

- Chrome for Testing `chrome-headless-shell` 151.0.7922.47 for Linux x64.
- Noto Sans SC from Noto Sans CJK 2.004.
- A private Fontconfig file that points Chrome to the bundled fonts.

The browser and fonts do not need to be installed separately:

```bash
./render-card -i ./card.dsl.jsonl
```

The full package still requires Node.js 22+ and the base shared libraries listed in `runtime/chrome-headless-shell/deb.deps`. On Ubuntu/Debian, install any missing libraries with the distribution package manager. Alpine/musl is not supported by this glibc Linux x64 build.

The official portable headless-shell archive does not include an installable setuid sandbox. The CLI therefore starts only the bundled runtime with `--no-sandbox`; a browser selected through `A2UI_BROWSER_PATH` or a normal system browser keeps its standard sandbox behavior. Render only trusted DSL and assets with the full portable package, especially on shared hosts.

For a non-standard browser path or an intentional override:

```bash
A2UI_BROWSER_PATH=/opt/chrome/chrome ./render-card -i ./card.dsl.jsonl
```
