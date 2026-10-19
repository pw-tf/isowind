#!/bin/sh
# Assembles Isowind for Linux the way package/make_package.ps1 does for Windows: the engine with the libraries it
# needs, the mod, the bundled mods, pixel art, profiles, server, licenses, readme, and isowind.sh to start it all.
#
#   package.sh BUILD_DIR OUT_DIR [BUNDLED_MODS_DIR]
#
# Needs linuxdeploy on PATH (it copies in every library that isn't part of the base system, so the folder runs on
# other distributions too).
set -e
SRC=$(cd "$(dirname "$0")/../.." && pwd)
I="$SRC/isowind"
BUILD=$(cd "$1" && pwd)
OUT=$2
MODS=$3

rm -rf "$OUT"
mkdir -p "$OUT"

# Engine: engine/usr/bin with its config, settings defaults and resources beside it, libraries in engine/usr/lib
linuxdeploy --appdir "$OUT/engine" -e "$BUILD/openmw" -e "$BUILD/tes3mp-server" -e "$BUILD/openmw-iniimporter"
cp -r "$BUILD/resources" "$BUILD/openmw.cfg" "$BUILD/defaults.bin" "$BUILD/gamecontrollerdb.txt" "$OUT/engine/usr/bin/"

cp -r "$I/mod" "$I/pixelart" "$OUT/"
for look in pixelart normal; do
    mkdir -p "$OUT/profiles/$look"
    cp "$I/profiles/$look/"* "$I/profiles/input_v3.xml" "$OUT/profiles/$look/"
done
mkdir -p "$OUT/server" "$OUT/tools" "$OUT/licenses"
cp -r "$I/server/CoreScripts" "$I/server/tes3mp-server-default.cfg" "$OUT/server/"
cp "$I/tools/retro_textures.py" "$OUT/tools/"
cp "$SRC/LICENSE" "$OUT/licenses/GPL-3.0.txt"
cp "$SRC/AUTHORS.md" "$OUT/licenses/OpenMW-AUTHORS.md"
cp "$I/README.txt" "$OUT/"
cp "$I/linux/isowind.sh" "$I/linux/isowind-launcher.py" "$OUT/"
chmod +x "$OUT/isowind.sh"

if [ -n "$MODS" ] && [ -d "$MODS" ]; then
    cp -r "$MODS" "$OUT/mods"
else
    echo "warning: no bundled mods given; the profiles expect them in mods/, and servers require their plugins" >&2
fi
