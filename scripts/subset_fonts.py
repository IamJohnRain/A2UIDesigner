#!/usr/bin/env python3
"""Regenerate the subsetted HarmonyOS Sans SC fonts used by the web renderer.

The full TTFs (~8 MB each) are subsetted to the characters actually used:
ASCII + Latin-1, GB2312 (level 1 & 2), CJK/full-width punctuation, common
symbols, plus every character found in the repo's cards/docs. This shrinks
the GitHub Pages artifact by ~37 MB while rendering pixel-identically.

Requires: pip install fonttools

Usage (from the repository root):
    python scripts/subset_fonts.py

The fonts are overwritten in place. Restore originals from git if needed
(`git checkout -- references/fonts`) and keep a local backup when iterating.
"""

from __future__ import annotations

import pathlib
import subprocess
import sys


ROOT = pathlib.Path(__file__).resolve().parent.parent
FONTS_DIR = ROOT / "references" / "fonts"
FONT_WEIGHTS = ["Thin", "Light", "Regular", "Medium", "Bold", "Black"]
CORPUS_TMP = ROOT / "tmp" / "font_subset.txt"


def build_corpus() -> str:
    chars: set[str] = set()
    # ASCII printable
    chars.update(chr(c) for c in range(0x20, 0x7F))
    # Latin-1, punctuation, arrows, enclosed alphanumerics, geometric shapes, misc symbols
    for lo, hi in [
        (0x00A0, 0x00FF),
        (0x2000, 0x206F),
        (0x2190, 0x21FF),
        (0x2460, 0x24FF),
        (0x2500, 0x25FF),
        (0x2600, 0x26FF),
        (0x3000, 0x303F),
        (0xFE10, 0xFE1F),
        (0xFF00, 0xFFEF),
    ]:
        chars.update(chr(c) for c in range(lo, hi + 1))
    # GB2312 level 1 & 2
    for b1 in range(0xA1, 0xF8):
        for b2 in range(0xA1, 0xFF):
            try:
                chars.add(bytes([b1, b2]).decode("gb2312"))
            except UnicodeDecodeError:
                pass
    # Project corpus (cards, docs, sources)
    roots = [
        ROOT / "tmp" / "run_luna_v4",
        ROOT / "docs",
        ROOT / "scripts",
    ]
    for root in roots:
        if not root.exists():
            continue
        for path in root.rglob("*"):
            if path.suffix.lower() in {".txt", ".md", ".json", ".jsonl", ".py", ".js"} and path.is_file():
                try:
                    chars.update(path.read_text(encoding="utf-8"))
                except UnicodeDecodeError:
                    pass
    for name in ["index.html", "app.js", "README.md", "AGENTS.md", "genui-renderer.js"]:
        path = ROOT / name
        if path.exists():
            try:
                chars.update(path.read_text(encoding="utf-8"))
            except UnicodeDecodeError:
                pass
    for ch in "\n\r\t":
        chars.discard(ch)
    return "".join(sorted(chars))


def main() -> int:
    if not (FONTS_DIR / "HarmonyOS_Sans_SC_Regular.ttf").exists():
        print(f"fonts not found under {FONTS_DIR}")
        return 1
    CORPUS_TMP.parent.mkdir(parents=True, exist_ok=True)
    corpus = build_corpus()
    CORPUS_TMP.write_text(corpus, encoding="utf-8")
    print(f"corpus: {len(corpus)} chars -> {CORPUS_TMP}")
    for weight in FONT_WEIGHTS:
        font = FONTS_DIR / f"HarmonyOS_Sans_SC_{weight}.ttf"
        result = subprocess.run(
            [
                sys.executable,
                "-m",
                "fontTools.subset",
                str(font),
                f"--text-file={CORPUS_TMP}",
                f"--output-file={font}",
                "--layout-features=*",
                "--name-IDs=*",
                "--symbol-cmap",
                "--recommended-glyphs",
            ],
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            print(f"subset failed for {font.name}:\n{result.stdout}\n{result.stderr}")
            return 1
        print(f"subset: {font.name} -> {font.stat().st_size / 1048576:.2f} MB")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
