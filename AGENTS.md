# Repository Guidelines

## Project Structure & Module Organization

This repository is a dependency-free, static Harmony Card DSL editor.

- `index.html` defines the three-panel editor shell and loads browser assets.
- `app.js` contains JSONL parsing, data binding, component rendering, drag/resize editing, history, and DSL export logic.
- `styles.css` contains the main responsive layout and canvas styling.
- `color-picker.css` contains the reusable color-palette UI.
- `references/media/` stores preview assets mapped from DSL paths such as `resources/base/media/icon_schedule.png`.
- `references/datasets/` contains sample cases with DSL files and expected PNG renders.
- `references/harmony-card-generation-datamodel-first/` documents the supported protocol and validation rules.
- `scripts/alt_to_dsl_converter.py` is the standalone ALT→DSL converter (TaskSpec + ALT + ASC in, DSL out; supports `--theme/--width/--height`). It is pure Python stdlib and is also executed in-browser via Pyodide.
- `scripts/config/` holds the ALT themes/layout/tuning JSON consumed both by the CLI and by the browser runtime through `configure_runtime`.
- `scripts/subset_fonts.py` regenerates the subsetted HarmonyOS Sans SC fonts under `references/fonts/` (full TTFs are ~8 MB each; the subset is ~1.7 MB each and renders pixel-identically). Requires `pip install fonttools`.

The Pyodide runtime is **not vendored**: `vendor/pyodide/` was removed to keep the GitHub Pages artifact small. The browser ALT tab lazy-loads the pinned runtime (core 0.26.4, including a `__tzset_js` locale patch) from the separate repo `IamJohnRain/a2ui-pyodide` — GitHub Pages primary source plus `raw.githubusercontent.com` fallback, both selected in `app.js` via `PYODIDE_BASES`. Upgrading/syncing that runtime is documented in the runtime repo's README and in `docs/alt-protocol-pyodide-plan.md` (section 5.2); the patch is reproducible with `patch-tzset.py` from that repo.

The ALT tab also supports BYOK LLM generation (user query → TaskSpec → ALT/ASC) through OpenAI-compatible `chat/completions` endpoints. Settings (BaseURL/model/API Key) live in `localStorage`; the API Key is encrypted with WebCrypto (PBKDF2-SHA-256 310000 iterations → AES-GCM) and decrypted only with the user master password, which is never persisted. The TaskSpec prompt template is plain Markdown (`scripts/alt-prompts/task-spec-generation.md` plus `reference/` appendix files) and is fetched at generation time. The ALT/ASC generation context is built by executing `scripts/taskspec_to_alt_chat_completions.py` inside the already-loaded Pyodide runtime (`build_request` → system + user messages), so the browser context is byte-identical to the training/eval pipeline; do not edit that script's output contract separately. `scripts/alt-prompts/alt-asc-generation.md` is a review-only mirror of its `SYSTEM_PROMPT` and is not used at runtime.

There is no generated build directory or package manager output.

## Config & Settings Synchronization

The settings page is metadata-driven: the 渲染参数 tab renders exactly the parameters declared in
`scripts/config/alt-tuning.meta.json`, while their defaults come from `scripts/config/alt-tuning.json`
(cloud defaults; user overrides live in `localStorage["a2ui.tuning.v1"]`). Whenever a component is added,
or any geometry/measurement behavior changes, update the following together in the same change:

- `scripts/config/alt-tuning.json` — the new or changed hyperparameter values.
- `scripts/config/alt-tuning.meta.json` — one entry per tunable parameter with a Chinese `name`,
  `category`/`group`, hover `tooltip`, control `type`, and `min/max/step/unit`. Without an entry here
  the parameter never appears in the 设置 UI.
- `scripts/config/alt-layout-profile.json` — structural layout rules (canvas / limits / spacing /
  textRules / typography) when they are affected.
- `scripts/config/alt-themes.json` — theme tokens when the component introduces colors.
- `scripts/alt_converter.py` and `scripts/alt_to_dsl_converter.py` — identical copies of the ALT-to-DSL
  pipeline (CLI batch vs browser/Pyodide); apply the same edit to both.
- `scripts/taskspec_to_alt_chat_completions.py` — when prompt-side units, limits, or allowed components
  change.
- `genui-renderer.js` — keep browser fallback defaults aligned with the tuning values so DSL-omitted
  styles render consistently.
- `docs/alt-tuning-parameters.md` — document every new parameter.

Verify every such change with:

1. `node --check app.js` and `git diff --check`.
2. Open 设置 → 渲染参数: the parameter must appear with a Chinese name, tooltip, and correct default;
   exercise 保存 → 刷新 → 自动加载 → 重置.
3. Recompile a representative card and confirm output is unchanged unless the change intends otherwise.
4. UI text must stay UTF-8: never pipe Chinese through shell pipelines that can replace non-ASCII with `?`,
   and scan `index.html`, `app.js`, and `scripts/config/*.json` for stray `?` runs before committing.
5. Bump the asset cache-buster query strings in `index.html` whenever `app.js` or CSS behavior changes,
   because GitHub Pages caches those assets.

## Build, Test, and Development Commands

No build step is required. Open `index.html` directly, or serve the repository locally:

```powershell
python -m http.server 8000
```

Then visit `http://localhost:8000/`.

Before committing, run:

```powershell
node --check app.js
git diff --check
```

The first command validates JavaScript syntax; the second detects whitespace errors. GitHub Pages publishes the `master` branch root automatically.

The in-browser ALT conversion needs network access on first use: clicking "编译并渲染" downloads ~7 MB of Pyodide runtime from `a2ui-pyodide` (GitHub Pages, falling back to raw). The rest of the editor works fully offline.

The Pages deploy only packages the site runtime files (see `.github/workflows/pages.yml` "Stage site files"): `index.html`, `app.js`, the CSS files, `genui-renderer.js`, `references/media/` (preview assets + `assets.js`), `references/fonts/` (subsetted fonts), `scripts/alt_to_dsl_converter.py`, `scripts/taskspec_to_alt_chat_completions.py`, `scripts/config/` and `scripts/alt-prompts/`. Everything else in the repo — including reference source such as `references/genui/`, `docs/`, `cli/`, tests and `__pycache__` — stays in git but is intentionally excluded from the deployed artifact.

## Coding Style & Naming Conventions

Use two-space indentation in HTML, CSS, and JavaScript. Prefer `camelCase` for JavaScript variables and functions, descriptive component IDs such as `action_button`, and kebab-case CSS classes such as `.color-picker`. Keep the project dependency-free and browser-native. Preserve DSL protocol values rather than introducing editor-only fields.

Preview-only asset paths must map to `references/media/`; exported DSL must retain `resources/base/media/...` paths.

## Testing Guidelines

There is no automated test framework. Validate changes against representative files in `references/datasets/`, especially components affected by the change. Compare the canvas with the case’s `card.dsl.png`, verify exported JSONL, and check the browser console for errors. Test both `140x140` and `300x140` cards when changing layout behavior.

## Commit & Pull Request Guidelines

Recent commits use short, imperative subjects, for example `Add visual background color picker` and `Match checkbox rendering to DSL protocol`. Keep each commit focused.

Pull requests should include a concise summary, affected cases/components, verification steps, and before/after screenshots for visual changes. Note any protocol, asset-mapping, or GitHub Pages cache implications.
