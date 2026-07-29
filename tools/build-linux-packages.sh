#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
DIST=${1:-"$ROOT/dist-linux"}
VERSION=$(cd "$ROOT" && node -p "require('./package.json').version")
CHROME_VERSION=151.0.7922.47
CHROME_URL="https://storage.googleapis.com/chrome-for-testing-public/$CHROME_VERSION/linux64/chrome-headless-shell-linux64.zip"
CHROME_SHA256=e60f546a64ecc6b5b5ddbde7b47f1304fc8fdba9ea65fd63c16bbc994787b3d5
FONT_VERSION=Sans2.004
FONT_URL="https://github.com/notofonts/noto-cjk/releases/download/$FONT_VERSION/18_NotoSansSC.zip"
FONT_SHA256=4d107c09ada479d3e48b6e78c83835773cbd9214bf6e12cdb7b60f8e068292ec

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
mkdir -p "$DIST"

download() {
  local url=$1 destination=$2 checksum=$3
  curl --fail --location --retry 3 --retry-delay 2 --output "$destination" "$url"
  echo "$checksum  $destination" | sha256sum --check --status
}

copy_common() {
  local target=$1
  mkdir -p "$target/cli" "$target/references"
  cp "$ROOT/genui-renderer.js" "$ROOT/package.json" "$ROOT/README.md" "$ROOT/render-card" "$ROOT/render-card.cmd" "$target/"
  cp -R "$ROOT/cli/." "$target/cli/"
  cp -R "$ROOT/references/media" "$target/references/"
  chmod +x "$target/render-card" "$target/cli/render-card.js"
}

LIGHT_ROOT="$WORK/light/a2ui-card-renderer"
FULL_ROOT="$WORK/full/a2ui-card-renderer"
copy_common "$LIGHT_ROOT"
mkdir -p "$WORK/full"
cp -R "$LIGHT_ROOT" "$FULL_ROOT"

CHROME_ZIP="$WORK/chrome-headless-shell-linux64.zip"
FONT_ZIP="$WORK/NotoSansSC.zip"
if [[ -n "${A2UI_CHROME_ARCHIVE:-}" ]]; then
  cp "$A2UI_CHROME_ARCHIVE" "$CHROME_ZIP"
  echo "$CHROME_SHA256  $CHROME_ZIP" | sha256sum --check --status
else
  download "$CHROME_URL" "$CHROME_ZIP" "$CHROME_SHA256"
fi
if [[ -n "${A2UI_FONT_ARCHIVE:-}" ]]; then
  cp "$A2UI_FONT_ARCHIVE" "$FONT_ZIP"
  echo "$FONT_SHA256  $FONT_ZIP" | sha256sum --check --status
else
  download "$FONT_URL" "$FONT_ZIP" "$FONT_SHA256"
fi

mkdir -p "$WORK/chrome" "$WORK/fonts" "$FULL_ROOT/runtime/chrome-headless-shell" "$FULL_ROOT/runtime/fonts"
unzip -q "$CHROME_ZIP" -d "$WORK/chrome"
unzip -q "$FONT_ZIP" -d "$WORK/fonts"
cp -R "$WORK/chrome/chrome-headless-shell-linux64/." "$FULL_ROOT/runtime/chrome-headless-shell/"
/usr/bin/find "$WORK/fonts" -type f \( -iname '*.ttf' -o -iname '*.otf' -o -iname '*.ttc' \) -exec cp '{}' "$FULL_ROOT/runtime/fonts/" \;
/usr/bin/find "$WORK/fonts" -type f \( -iname 'LICENSE*' -o -iname 'OFL*' \) -exec cp '{}' "$FULL_ROOT/runtime/fonts/" \;
test -n "$(/usr/bin/find "$FULL_ROOT/runtime/fonts" -type f \( -iname '*.ttf' -o -iname '*.otf' -o -iname '*.ttc' \) -print -quit)"
chmod +x "$FULL_ROOT/runtime/chrome-headless-shell/chrome-headless-shell"

cat > "$FULL_ROOT/runtime/fontconfig.xml" <<'EOF'
<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "fonts.dtd">
<fontconfig>
  <dir prefix="relative">fonts</dir>
  <cachedir prefix="xdg">fontconfig</cachedir>
  <match target="pattern">
    <test name="family" qual="any"><string>sans-serif</string></test>
    <edit name="family" mode="prepend" binding="strong"><string>Noto Sans SC</string></edit>
  </match>
</fontconfig>
EOF

cat > "$FULL_ROOT/runtime/VERSIONS.txt" <<EOF
chrome-headless-shell $CHROME_VERSION
Noto Sans CJK $FONT_VERSION (NotoSansSC)
EOF

LIGHT_ARCHIVE="$DIST/a2ui-card-renderer-linux-x64-light-v$VERSION.tar.gz"
FULL_ARCHIVE="$DIST/a2ui-card-renderer-linux-x64-full-v$VERSION.tar.gz"
tar -czf "$LIGHT_ARCHIVE" -C "$WORK/light" a2ui-card-renderer
tar -czf "$FULL_ARCHIVE" -C "$WORK/full" a2ui-card-renderer
(cd "$DIST" && sha256sum "$(basename "$LIGHT_ARCHIVE")" "$(basename "$FULL_ARCHIVE")" > SHA256SUMS-linux.txt)
printf '%s\n' "$LIGHT_ARCHIVE" "$FULL_ARCHIVE" "$DIST/SHA256SUMS-linux.txt"
