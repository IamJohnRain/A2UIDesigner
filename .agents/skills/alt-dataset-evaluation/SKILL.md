---
name: alt-dataset-evaluation
description: "Run the 60-case ALT evaluation pipeline in this A2UI Designer repository: migrate TaskSpec PNG assets to declared local SVGs, build chat-completions requests, call the configured gpt_5_6_luna model for ALT/ASC, compile DSL, render PNG previews, and audit layout/color failures. Use when evaluating or revising the automatic ALT protocol against references/vals."
---

# ALT dataset evaluation

Use this skill when a protocol change must be tested against every Case in `references/vals`. Keep source DSL/PNG artifacts intact, put generated artifacts beside each Case with a distinct suffix, and do not expose or commit `.env.toml`.

The workflow is staged: asset migration, request construction, model generation, ALT compilation, browser rendering, and visual/structural audit. A SubAgent should execute the long-running batch; the parent agent reviews representative outputs and decides protocol changes.

## Preconditions

- Work from the repository root.
- Confirm there are 60 immediate Case directories and each has `task.taskSpec.json`.
- Confirm `references/media/` contains the SVG catalog and `scripts/run_alt_dataset.py` is available.
- Confirm `.env.toml` has a `gpt_5_6_luna` table. Never print the file or API key in logs, prompts, summaries, commits, or final responses.
- Ensure Python, Node.js, and the browser used by `cli/render-card.js` are available.

## SubAgent handoff

Delegate the complete batch to one SubAgent with a concrete request similar to:

> In the shared repository, process all 60 Cases under `references/vals`. Preserve original DSL/PNG. Migrate TaskSpec PNG asset paths to matching local SVGs, build one request body per Case with `scripts/taskspec_to_alt_chat_completions.py`, aggregate them for `scripts/run_alt_dataset.py -m gpt_5_6_luna`, copy generated ALT/ASC back with a `luna` suffix, compile with `scripts/alt_converter.py` into a suffixed DSL/report, render each DSL with `node cli/render-card.js`, and return counts plus representative layout/color findings. Do not commit `.env.toml` or overwrite original files.

The SubAgent must report the exact output root, failed Case names, and commands/results; it must not silently downgrade SVG mapping or treat a failed model response as a valid Case.

## Pipeline

### 1. Migrate assets without losing provenance

For each TaskSpec, recursively inspect `assetCandidates`, `dataModelSchema.sampleValue`, and any asset `bindTo` samples. Map a `.png` only when a same-stem `.svg` exists in `references/media/` or an explicit semantic mapping can be justified from the catalog. Record every replacement in a case-local `asset-svg-migration.json` and copy the original once to `task.taskSpec.before-svg.json`; do not invent paths. Validate that no generated TaskSpec asset source ends in `.png`, `.jpg`, `.jpeg`, or a network/data URL.

Use a distinct generated TaskSpec filename if the source dataset must remain byte-for-byte unchanged. The model request must reference the migrated TaskSpec, while the original remains the comparison baseline.

### 2. Build RequestBody files

Existing Case ALT files may use the legacy geometry protocol. First run `scripts/alt_converter.py -o references/vals --mode d2t --allow-layout-issues` with distinct names such as `card.luna.source.alt.txt` and `card.luna.source.asc.txt`; this converts each reachable source DSL into automatic ALT plus its semantic companion while writing diagnostics. Then run `scripts/taskspec_to_alt_chat_completions.py --allow-layout-issues` for every migrated Case using those automatic source files and its query. These flags allow known-bad legacy references into the Assistant example only; they do not weaken validation of model outputs or downstream DSL compilation. Build an aggregate JSONL containing one request per Case for the model runner. The generated Assistant contract is raw `<alt>...</alt>` followed by `<asc>...</asc>`; reject headers, Markdown fences, `<think>`, DSL, colors, dimensions, and DataModel copies.

### 3. Generate ALT/ASC with the model

Run from the repository root, for example:

```powershell
python scripts/run_alt_dataset.py `
  --input <aggregate-request.jsonl> `
  --output-dir <generated-root> `
  --model-label gpt_5_6_luna `
  --env-toml .env.toml `
  --jobs 4 `
  --debug
```

Use `--limit`, `--start`, or `--end` for a smoke test before all 60. Keep `--debug` output outside tracked paths when responses may contain sensitive data. Verify every generated directory has non-empty `card.alt.txt` and `card.asc.txt` and that the runner summary has no API or parse failures.

### 4. Compile without overwriting the baseline

Map runner `case-001`...`case-060` back to the sorted source Case names. Copy generated model ALT/ASC into each source Case using `card.luna.model.alt.txt` and `card.luna.model.asc.txt`. Compile with:

```powershell
python scripts/alt_converter.py -o references/vals --mode t2d `
  --alt_name card.luna.model.alt.txt --asc_name card.luna.model.asc.txt `
  --dsl_name card.luna.dsl.jsonl --layout_report_name card.luna.layout-report.txt
```

Never replace `card.dsl.jsonl`, `card.dsl.png`, `card.alt.txt`, or `card.asc.txt` during evaluation. A hard layout error must leave the original DSL untouched and produce a report explaining the failure.

### 5. Render generated DSL

For each successful generated DSL:

```powershell
node cli/render-card.js -i <case>\card.luna.dsl.jsonl `
  -o <case> -n card.luna.dsl.png
```

Check the renderer console for missing assets and browser errors. The renderer must resolve preview assets from `references/media/`; exported DSL paths remain `resources/base/media/*.svg`.

## Evaluation and protocol iteration

Aggregate `card.luna.layout-report.txt` statuses and inspect at least one 2x2 and one 2x4 Case for every major scenario. Use `view_image` on generated PNGs and compare with the Case reference PNG where present. Classify findings as:

- text overflow, clipping, or button-label occlusion;
- component overlap, orphan/unmounted nodes, or unsafe whitespace;
- incorrect 2x2/2x4 density or vertical anchoring;
- SVG missing/incorrect `fillColor` or insufficient contrast;
- inconsistent theme surface/text/action colors.

Use the reports plus visual evidence to change only the automatic protocol/compiler/profile/theme mapping and prompt constraints. Do not make the model specify width, height, font size, spacing, radius, or colors. Prefer tightening limits or removing optional roles over adding per-Case exceptions. Re-run a small representative subset, then the full 60-case pipeline.

Final report must include total/converted/rendered/failed counts, failed Case names with reasons, layout issue histogram, color/contrast findings, representative PNG paths, protocol changes, and test commands. Keep secrets and raw API responses out of the final response and Git history.
