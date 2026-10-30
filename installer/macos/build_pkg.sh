#!/bin/bash
# Build VialFX-Bundle-Setup-macos.pkg from a folder containing VST3/ and CLAP/ (plus LICENSE).
# Usage: installer/macos/build_pkg.sh <bundle-dir> <output-dir> [version]
set -euo pipefail
SRC="$(cd "$1" && pwd)"; OUT="$(cd "$2" && pwd)"; VERSION="${3:-0.1.0}"
WORK="$(mktemp -d)"
EFFECTS=(Chorus Compressor Delay Distortion EQ Filter Flanger Phaser Reverb)
mkdir -p "$WORK/pkgs" "$WORK/resources"
cp "$SRC/LICENSE" "$WORK/resources/LICENSE.txt"

lower() { echo "$1" | tr '[:upper:]' '[:lower:]'; }

build_component() {  # format ext dest
  local fmt="$1" ext="$2" dest="$3" e id root
  for e in "${EFFECTS[@]}"; do
    id="com.vialfx.$fmt.$(lower "$e")"
    root="$WORK/root-$fmt-$(lower "$e")"
    mkdir -p "$root"
    ditto "$SRC/$(echo "$fmt" | tr a-z A-Z)/Vial $e.$ext" "$root/Vial $e.$ext"
    pkgbuild --root "$root" --install-location "$dest" --identifier "$id" --version "$VERSION" \
             "$WORK/pkgs/$fmt-$(lower "$e").pkg" >/dev/null
  done
}
build_component vst3 vst3 "/Library/Audio/Plug-Ins/VST3"
build_component clap clap "/Library/Audio/Plug-Ins/CLAP"

{
  echo '<?xml version="1.0" encoding="utf-8"?>'
  echo '<installer-gui-script minSpecVersion="1">'
  echo '  <title>VialFX-Bundle</title>'
  echo '  <license file="LICENSE.txt"/>'
  echo '  <options customize="always" require-scripts="false" hostArchitectures="x86_64,arm64"/>'
  echo '  <choices-outline>'
  for fmt in vst3 clap; do
    echo "    <line choice=\"$fmt\">"
    for e in "${EFFECTS[@]}"; do echo "      <line choice=\"$fmt.$(lower "$e")\"/>"; done
    echo '    </line>'
  done
  echo '  </choices-outline>'
  echo '  <choice id="vst3" title="VST3 plugins" description="Installed to /Library/Audio/Plug-Ins/VST3"/>'
  echo '  <choice id="clap" title="CLAP plugins" description="Installed to /Library/Audio/Plug-Ins/CLAP"/>'
  for fmt in vst3 clap; do
    for e in "${EFFECTS[@]}"; do
      id="com.vialfx.$fmt.$(lower "$e")"
      echo "  <choice id=\"$fmt.$(lower "$e")\" title=\"Vial $e\" start_selected=\"true\"><pkg-ref id=\"$id\"/></choice>"
      echo "  <pkg-ref id=\"$id\" version=\"$VERSION\" onConclusion=\"none\">$fmt-$(lower "$e").pkg</pkg-ref>"
    done
  done
  echo '</installer-gui-script>'
} > "$WORK/distribution.xml"

productbuild --distribution "$WORK/distribution.xml" --package-path "$WORK/pkgs" \
             --resources "$WORK/resources" "$OUT/VialFX-Bundle-Setup-macos.pkg"
rm -rf "$WORK"
echo "Built $OUT/VialFX-Bundle-Setup-macos.pkg"
