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

There is no generated build directory or package manager output.

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

## Coding Style & Naming Conventions

Use two-space indentation in HTML, CSS, and JavaScript. Prefer `camelCase` for JavaScript variables and functions, descriptive component IDs such as `action_button`, and kebab-case CSS classes such as `.color-picker`. Keep the project dependency-free and browser-native. Preserve DSL protocol values rather than introducing editor-only fields.

Preview-only asset paths must map to `references/media/`; exported DSL must retain `resources/base/media/...` paths.

## Testing Guidelines

There is no automated test framework. Validate changes against representative files in `references/datasets/`, especially components affected by the change. Compare the canvas with the case’s `card.dsl.png`, verify exported JSONL, and check the browser console for errors. Test both `140x140` and `300x140` cards when changing layout behavior.

## Commit & Pull Request Guidelines

Recent commits use short, imperative subjects, for example `Add visual background color picker` and `Match checkbox rendering to DSL protocol`. Keep each commit focused.

Pull requests should include a concise summary, affected cases/components, verification steps, and before/after screenshots for visual changes. Note any protocol, asset-mapping, or GitHub Pages cache implications.
