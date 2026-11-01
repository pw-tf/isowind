#!/bin/sh
# Assembles Isowind for Linux the way isowind/windows/make_package.ps1 does for Windows: the engine with the
# libraries it needs, the mod, the bundled mods if any, pixel art, profiles, server, licenses, readme, and isowind.sh
# to start it all.
#
#   package.sh [--with-mods | --no-mods] BUILD_DIR OUT_DIR [BUNDLED_MODS_DIR]
#
# The bundled mods (third-party, kept out of git) go in when they are there: BUNDLED_MODS_DIR, or else
# build-deps/bundled-mods in the repository. Without them the package is made for OpenMW alone
# (isowind/tools/strip_mods.py, as make_package.ps1 does), as anyone building from the public repository gets.
#   --with-mods  fetches them into that folder first if it isn't there, from the repository's private "bundled-mods"
#                release (needs the GitHub CLI, logged in with access to it); every package after that has them
#   --no-mods    leaves them out even when they are there, for a public release
#
# Needs linuxdeploy on PATH (it copies in every library that isn't part of the base system, so the folder runs on
# other distributions too).
#
# OUT_DIR is laid down afresh, except for what hosting from it made, which is the host's: the server's saved players,
# cells and world (server/CoreScripts/data) and its settings (server/tes3mp-server.cfg), so packaging again keeps the
# test characters. Those and the single-player saves (~/.local/share/openmw/saves) are backed up first, in
# ~/.local/share/isowind/backups (the last five). A release is packaged into a new folder, as the CI does.
set -e
WANT_MODS=
case $1 in
    --with-mods) WANT_MODS=yes; shift ;;
    --no-mods) WANT_MODS=no; shift ;;
esac
SRC=$(cd "$(dirname "$0")/../.." && pwd)
I="$SRC/isowind"
BUILD=$(cd "$1" && pwd)
OUT=$2
MODS=${3:-$SRC/build-deps/bundled-mods}
DATA_HOME=${XDG_DATA_HOME:-$HOME/.local/share}

mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)
# What is in OUT goes (below): only a package from here, or a new or empty folder
if [ -n "$(ls -A "$OUT")" ] && [ ! -f "$OUT/isowind.sh" ]; then
    echo "error: $OUT has files in it and isn't an Isowind package; give a new or empty folder" >&2
    exit 1
fi

# The bundled mods: fetched when asked for and missing, packaged when there
if [ "$WANT_MODS" = yes ] && [ ! -d "$MODS" ]; then
    if ! command -v gh >/dev/null 2>&1; then
        echo "error: the bundled mods come from the repository's private release: install the GitHub CLI, run" \
            "gh auth login, and package again; or put the mods in $MODS yourself" >&2
        exit 1
    fi
    echo "Fetching the bundled mods (bundled-mods.zip from the repository's bundled-mods release)..."
    FETCH=$(mktemp -d)
    gh release download bundled-mods --repo pw-tf/isowind -p bundled-mods.zip -D "$FETCH"
    mkdir -p "$MODS"
    unzip -q "$FETCH/bundled-mods.zip" -d "$MODS"
    rm -rf "$FETCH"
fi
if [ "$WANT_MODS" = no ] || [ ! -d "$MODS" ]; then MODS=; fi
if [ -n "$MODS" ]; then echo "With the bundled mods ($MODS)"; else echo "Without the bundled mods"; fi

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

# OpenSceneGraph's plugins the engine reads its images, fonts and models with (USED_OSG_PLUGINS in CMakeLists.txt).
# OSG loads them at run time, so linuxdeploy doesn't see them: they come from beside the libosgDB the engine runs
# with, as CMake finds them, into engine/usr/lib/osgPlugins-<version>, where isowind.sh points OSG. Without them OSG
# would look in the build machine's folder for them, and on another distribution find none, or its own OSG's, which
# load that OSG beside the bundled one and give the engine no image readers
OSGDB=$(ldd "$BUILD/openmw" | awk '/libosgDB\.so/ { print $3; exit }')
OSG_PLUGINS=
for d in "${OSGDB%/*}"/osgPlugins-*; do if [ -d "$d" ]; then OSG_PLUGINS=$d; break; fi; done
[ -n "$OSG_PLUGINS" ] || { echo "error: no osgPlugins folder beside $OSGDB" >&2; exit 1; }
mkdir -p "$OUT/engine/usr/lib/${OSG_PLUGINS##*/}"
for plugin in $(sed -n '/^set(USED_OSG_PLUGINS/,/)/p' "$SRC/CMakeLists.txt" | grep -o 'osgdb_[a-z_]*'); do
    # (one whose libraries this machine hasn't, such as Collada's for osgdb_dae on Arch, can't load here either)
    if ldd "$OSG_PLUGINS/$plugin.so" | grep -q 'not found'; then
        echo "warning: $plugin left out, as this machine hasn't all its libraries:" \
            "$(ldd "$OSG_PLUGINS/$plugin.so" | awk '/not found/ { printf "%s ", $1 }')" >&2
        continue
    fi
    cp "$OSG_PLUGINS/$plugin.so" "$OUT/engine/usr/lib/${OSG_PLUGINS##*/}/"
done

# Engine: engine/usr/bin with its config, settings defaults and resources beside it, libraries in engine/usr/lib (the
# plugins' too)
linuxdeploy --appdir "$OUT/engine" -e "$BUILD/openmw" -e "$BUILD/tes3mp-server" -e "$BUILD/openmw-iniimporter" \
    --deploy-deps-only "$OUT/engine/usr/lib/${OSG_PLUGINS##*/}"
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
cp "$I/README.txt" "$OUT/"
cp "$I/linux/isowind.sh" "$I/linux/isowind-launcher.py" "$OUT/"
chmod +x "$OUT/isowind.sh"

if [ -n "$MODS" ]; then
    cp -r "$MODS" "$OUT/mods"
else
    # Without the bundled mods (as for a public release): the profiles, the server's plugin list and the readme made
    # for OpenMW alone
    python3 "$I/tools/strip_mods.py" "$OUT"
fi
