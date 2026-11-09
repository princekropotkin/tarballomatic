#!/bin/sh
# Builds TarballOMatic and creates a .hpkg. Run on Haiku.
set -e

VERSION=1.0
REVISION=1
ARCH=$(getarch)
ROOT=$(cd "$(dirname "$0")/.." && pwd)
STAGE="$ROOT/package/stage"
OUT="$ROOT/tarballomatic-$VERSION-$REVISION-$ARCH.hpkg"

make -C "$ROOT/tarballomatic"
BINARY=$(ls "$ROOT"/tarballomatic/objects.*/TarballOMatic | head -n 1)

rm -rf "$STAGE"
mkdir -p "$STAGE/add-ons/tracker" "$STAGE/data/expander/rules"
cp "$BINARY" "$STAGE/add-ons/tracker/TarballOMatic"
cp "$ROOT/expander-rules/all" "$STAGE/data/expander/rules/bsdtar"
sed -e "s/@VERSION@/$VERSION/g" -e "s/@REVISION@/$REVISION/g" \
	-e "s/@ARCH@/$ARCH/g" "$ROOT/package/PackageInfo.in" \
	> "$STAGE/.PackageInfo"

rm -f "$OUT"
package create -C "$STAGE" "$OUT"
package list "$OUT"
echo "Created $OUT"
