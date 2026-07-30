#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
DIST=${1:-"$ROOT/dist-linux"}
VERSION=$(cd "$ROOT" && node -p "require('./package.json').version")
CHROME_VERSION=151.0.7922.47
PLAYWRIGHT_REVISION=1235
CHROME_X64_URL="https://storage.googleapis.com/chrome-for-testing-public/$CHROME_VERSION/linux64/chrome-headless-shell-linux64.zip"
CHROME_X64_SHA256=e60f546a64ecc6b5b5ddbde7b47f1304fc8fdba9ea65fd63c16bbc994787b3d5
CHROME_ARM64_URL="https://cdn.playwright.dev/dbazure/download/playwright/builds/chromium/$PLAYWRIGHT_REVISION/chromium-headless-shell-linux-arm64.zip"
CHROME_ARM64_SHA256=1613713a1f836e4a8a24c25b01ae61f7a0a5eb0439389f3746211592f6144335
FONT_VERSION=Sans2.004
FONT_URL="https://github.com/notofonts/noto-cjk/releases/download/$FONT_VERSION/18_NotoSansSC.zip"
FONT_SHA256=4d107c09ada479d3e48b6e78c83835773cbd9214bf6e12cdb7b60f8e068292ec

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
mkdir -p "$DIST"

verify_checksum() {
  local destination=$1 expected=$2 actual
  actual=$(sha256sum "$destination" | cut -d ' ' -f 1)
  if [[ "$actual" != "$expected" ]]; then
    printf 'SHA-256 mismatch for %s\nexpected: %s\nactual:   %s\n' "$destination" "$expected" "$actual" >&2
    return 1
  fi
}

download() {
  local url=$1 destination=$2 checksum=$3
  curl --fail --location --retry 3 --retry-delay 2 --output "$destination" "$url"
  verify_checksum "$destination" "$checksum"
}

copy_common() {
  local target=$1
  mkdir -p "$target/cli" "$target/references"
  cp "$ROOT/genui-renderer.js" "$ROOT/package.json" "$ROOT/README.md" "$ROOT/render-card" "$ROOT/render-card.cmd" "$target/"
  cp -R "$ROOT/cli/." "$target/cli/"
  cp -R "$ROOT/references/media" "$ROOT/references/fonts" "$target/references/"
  chmod +x "$target/render-card" "$target/cli/render-card.js"
}

copy_archive_or_download() {
  local override=$1 url=$2 destination=$3 checksum=$4
  if [[ -n "$override" ]]; then
    cp "$override" "$destination"
    verify_checksum "$destination" "$checksum"
  else
    download "$url" "$destination" "$checksum"
  fi
}

CHROME_X64_ZIP="$WORK/chrome-headless-shell-linux64.zip"
CHROME_ARM64_ZIP="$WORK/chromium-headless-shell-linux-arm64.zip"
FONT_ZIP="$WORK/NotoSansSC.zip"
copy_archive_or_download "${A2UI_CHROME_X64_ARCHIVE:-${A2UI_CHROME_ARCHIVE:-}}" "$CHROME_X64_URL" "$CHROME_X64_ZIP" "$CHROME_X64_SHA256"
copy_archive_or_download "${A2UI_CHROME_ARM64_ARCHIVE:-}" "$CHROME_ARM64_URL" "$CHROME_ARM64_ZIP" "$CHROME_ARM64_SHA256"
copy_archive_or_download "${A2UI_FONT_ARCHIVE:-}" "$FONT_URL" "$FONT_ZIP" "$FONT_SHA256"

mkdir -p "$WORK/chrome-x64" "$WORK/chrome-arm64" "$WORK/fonts"
unzip -q "$CHROME_X64_ZIP" -d "$WORK/chrome-x64"
unzip -q "$CHROME_ARM64_ZIP" -d "$WORK/chrome-arm64"
unzip -q "$FONT_ZIP" -d "$WORK/fonts"

create_package_roots() {
  local architecture=$1 light_root=$2 full_root=$3
  copy_common "$light_root"
  mkdir -p "$(dirname "$full_root")"
  cp -R "$light_root" "$full_root"
  mkdir -p "$full_root/runtime/chrome-headless-shell" "$full_root/runtime/fonts"
  printf '%s\n' "$architecture" > "$full_root/runtime/ARCHITECTURE"
  /usr/bin/find "$WORK/fonts" -type f \( -iname '*.ttf' -o -iname '*.otf' -o -iname '*.ttc' \) -exec cp '{}' "$full_root/runtime/fonts/" \;
  /usr/bin/find "$WORK/fonts" -type f \( -iname 'LICENSE*' -o -iname 'OFL*' \) -exec cp '{}' "$full_root/runtime/fonts/" \;
  test -n "$(/usr/bin/find "$full_root/runtime/fonts" -type f \( -iname '*.ttf' -o -iname '*.otf' -o -iname '*.ttc' \) -print -quit)"
}

X64_LIGHT_ROOT="$WORK/x64-light/a2ui-card-renderer"
X64_FULL_ROOT="$WORK/x64-full/a2ui-card-renderer"
ARM64_LIGHT_ROOT="$WORK/arm64-light/a2ui-card-renderer"
ARM64_FULL_ROOT="$WORK/arm64-full/a2ui-card-renderer"
create_package_roots x64 "$X64_LIGHT_ROOT" "$X64_FULL_ROOT"
create_package_roots arm64 "$ARM64_LIGHT_ROOT" "$ARM64_FULL_ROOT"

cp -R "$WORK/chrome-x64/chrome-headless-shell-linux64/." "$X64_FULL_ROOT/runtime/chrome-headless-shell/"
cp -R "$WORK/chrome-arm64/chrome-linux/." "$ARM64_FULL_ROOT/runtime/chrome-headless-shell/"
mv "$ARM64_FULL_ROOT/runtime/chrome-headless-shell/headless_shell" "$ARM64_FULL_ROOT/runtime/chrome-headless-shell/chrome-headless-shell"
chmod +x "$X64_FULL_ROOT/runtime/chrome-headless-shell/chrome-headless-shell"
chmod +x "$ARM64_FULL_ROOT/runtime/chrome-headless-shell/chrome-headless-shell"

write_runtime_metadata() {
  local full_root=$1 source=$2
  cat > "$full_root/runtime/fontconfig.xml" <<'EOF'
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
  cat > "$full_root/runtime/VERSIONS.txt" <<EOF
chrome-headless-shell $CHROME_VERSION ($source)
Noto Sans CJK $FONT_VERSION (NotoSansSC)
EOF
}

write_runtime_metadata "$X64_FULL_ROOT" "Google Chrome for Testing"
write_runtime_metadata "$ARM64_FULL_ROOT" "Microsoft Playwright revision $PLAYWRIGHT_REVISION, non-CfT ARM64 build"

X64_LIGHT_ARCHIVE="$DIST/a2ui-card-renderer-linux-x64-light-v$VERSION.tar.gz"
X64_FULL_ARCHIVE="$DIST/a2ui-card-renderer-linux-x64-full-v$VERSION.tar.gz"
ARM64_LIGHT_ARCHIVE="$DIST/a2ui-card-renderer-linux-arm64-light-v$VERSION.tar.gz"
ARM64_FULL_ARCHIVE="$DIST/a2ui-card-renderer-linux-arm64-full-v$VERSION.tar.gz"
tar -czf "$X64_LIGHT_ARCHIVE" -C "$WORK/x64-light" a2ui-card-renderer
tar -czf "$X64_FULL_ARCHIVE" -C "$WORK/x64-full" a2ui-card-renderer
tar -czf "$ARM64_LIGHT_ARCHIVE" -C "$WORK/arm64-light" a2ui-card-renderer
tar -czf "$ARM64_FULL_ARCHIVE" -C "$WORK/arm64-full" a2ui-card-renderer
(cd "$DIST" && sha256sum a2ui-card-renderer-linux-*.tar.gz > SHA256SUMS-linux.txt)
printf '%s\n' "$X64_LIGHT_ARCHIVE" "$X64_FULL_ARCHIVE" "$ARM64_LIGHT_ARCHIVE" "$ARM64_FULL_ARCHIVE" "$DIST/SHA256SUMS-linux.txt"
