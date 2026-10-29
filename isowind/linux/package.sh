#!/bin/sh
# Assembles Isowind for Linux the way isowind/windows/make_package.ps1 does for Windows: the engine with the
# libraries it needs, the mod, the bundled mods, pixel art, profiles, server, licenses, readme, and isowind.sh to
# start it all.
#
#   package.sh BUILD_DIR OUT_DIR [BUNDLED_MODS_DIR]
#
# Without BUNDLED_MODS_DIR the package goes without the mods, made for OpenMW alone (isowind/tools/strip_mods.py, as
# make_package.ps1 -NoMods does), as for a public release.
#
# Needs linuxdeploy on PATH (it copies in every library that isn't part of the base system, so the folder runs on
# other distributions too).
#
# OUT_DIR is laid down afresh, except for what hosting from it made, which is the host's: the server's saved players,
# cells and world (server/CoreScripts/data) and its settings (server/tes3mp-server.cfg), so packaging again keeps the
# test characters. Those and the single-player saves (~/.local/share/openmw/saves) are backed up first, in
# ~/.local/share/isowind/backups (the last five). A release is packaged into a new folder, as the CI does.
set -e
SRC=$(cd "$(dirname "$0")/../.." && pwd)
I="$SRC/isowind"
BUILD=$(cd "$1" && pwd)
OUT=$2
MODS=$3
DATA_HOME=${XDG_DATA_HOME:-$HOME/.local/share}

mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)
# What is in OUT goes (below): only a package from here, or a new or empty folder
if [ -n "$(ls -A "$OUT")" ] && [ ! -f "$OUT/isowind.sh" ]; then
    echo "error: $OUT has files in it and isn't an Isowind package; give a new or empty folder" >&2
    exit 1
fi

# Backup of what testing made (not while there is nothing yet but the server's defaults)
backup() {
    STATE=$(find "$OUT/server/CoreScripts/data" "$OUT/server/tes3mp-server.cfg" "$DATA_HOME/openmw/saves" -type f \
        ! -name .gitkeep ! -path "$OUT/server/CoreScripts/data/banlist.json" \
        ! -path "$OUT/server/CoreScripts/data/requiredDataFiles.json" 2>/dev/null | head -n 1)
    if [ -z "$STATE" ]; then return 0; fi
    set --
    for f in server/CoreScripts/data server/tes3mp-server.cfg; do
        if [ -e "$OUT/$f" ]; then set -- "$@" -C "$OUT" "$f"; fi
    done
    if [ -d "$DATA_HOME/openmw/saves" ]; then set -- "$@" -C "$DATA_HOME/openmw" saves; fi
    BACKUPS="$DATA_HOME/isowind/backups"
    mkdir -p "$BACKUPS"
    BACKUP="$BACKUPS/isowind-data-$(date +%Y%m%d-%H%M%S).tar.gz"
    tar -czf "$BACKUP" "$@"
    ls "$BACKUPS" | grep -E '^isowind-data-[0-9]{8}-[0-9]{6}\.tar\.gz$' | sort -r | tail -n +6 |
        while IFS= read -r old; do rm -f "$BACKUPS/$old"; done
    echo "Backup: $BACKUP"
}
backup

# Everything but the host's server data and settings goes
find "$OUT" -mindepth 1 -maxdepth 1 ! -name server -exec rm -rf {} +
if [ -d "$OUT/server" ]; then
    find "$OUT/server" -mindepth 1 -maxdepth 1 ! -name CoreScripts ! -name tes3mp-server.cfg -exec rm -rf {} +
fi
if [ -d "$OUT/server/CoreScripts" ]; then
    find "$OUT/server/CoreScripts" -mindepth 1 -maxdepth 1 ! -name data -exec rm -rf {} +
fi

# Engine: engine/usr/bin with its config, settings defaults and resources beside it, libraries in engine/usr/lib
linuxdeploy --appdir "$OUT/engine" -e "$BUILD/openmw" -e "$BUILD/tes3mp-server" -e "$BUILD/openmw-iniimporter"
cp -r "$BUILD/resources" "$BUILD/openmw.cfg" "$BUILD/defaults.bin" "$BUILD/gamecontrollerdb.txt" "$OUT/engine/usr/bin/"

cp -r "$I/mod" "$I/pixelart" "$OUT/"
for look in pixelart normal; do
    mkdir -p "$OUT/profiles/$look"
    cp "$I/profiles/$look/"* "$I/profiles/input_v3.xml" "$OUT/profiles/$look/"
done
mkdir -p "$OUT/server/CoreScripts" "$OUT/tools" "$OUT/licenses"
cp "$I/server/tes3mp-server-default.cfg" "$OUT/server/"
# CoreScripts with Isowind's changes. Of its data, the repository's as git has it (the empty folders, the ban list,
# the plugin list; not whatever a server run in the repository left there), and only what the folder doesn't have
# yet, but for the plugin list, which goes with the bundled mods
for f in "$I/server/CoreScripts"/* "$I/server/CoreScripts"/.[!.]*; do
    case ${f##*/} in data) ;; *) if [ -e "$f" ]; then cp -r "$f" "$OUT/server/CoreScripts/"; fi ;; esac
done
DATA_FILES=$(git -C "$SRC" ls-files isowind/server/CoreScripts/data 2>/dev/null | sed 's|^isowind/server/CoreScripts/||')
if [ -z "$DATA_FILES" ]; then
    DATA_FILES=$(cd "$I/server/CoreScripts" && { find data -type f -name .gitkeep; find data -maxdepth 1 -type f -name '*.json'; })
fi
printf '%s\n' "$DATA_FILES" | while IFS= read -r f; do
    if [ ! -e "$OUT/server/CoreScripts/$f" ] || [ "$f" = data/requiredDataFiles.json ]; then
        mkdir -p "$OUT/server/CoreScripts/${f%/*}"
        cp "$I/server/CoreScripts/$f" "$OUT/server/CoreScripts/$f"
    fi
done
cp "$I/tools/retro_textures.py" "$OUT/tools/"
cp "$SRC/LICENSE" "$OUT/licenses/GPL-3.0.txt"
cp "$SRC/AUTHORS.md" "$OUT/licenses/OpenMW-AUTHORS.md"
cp "$I/README.txt" "$OUT/"
cp "$I/linux/isowind.sh" "$I/linux/isowind-launcher.py" "$OUT/"
chmod +x "$OUT/isowind.sh"

if [ -n "$MODS" ] && [ -d "$MODS" ]; then
    cp -r "$MODS" "$OUT/mods"
else
    # Without the bundled mods (as for a public release): the profiles, the server's plugin list and the readme made
    # for OpenMW alone
    python3 "$I/tools/strip_mods.py" "$OUT"
fi
