#!/bin/sh
# Isowind on Linux: what Isowind.exe does on Windows.
#
#   ./isowind.sh                  play (pixel art look, or the look you used last)
#   ./isowind.sh normal           play with the normal look (or: pixelart)
#   ./isowind.sh join ADDRESS     play on a server, e.g. ./isowind.sh join 203.0.113.7:25565 (look optional after it)
#   ./isowind.sh host             run a server here, in this terminal (others join with this computer's address)
#
# The first time, it finds Morrowind (Steam's usual places; otherwise run it as MORROWIND=/path/to/Morrowind
# ./isowind.sh, the folder with "Data Files" and Morrowind.ini) and sets up OpenMW for it, like OpenMW's wizard.
# The pixel art look builds its textures from your Morrowind files the first time (needs Python 3 with Pillow).
set -e
ROOT=$(cd "$(dirname "$0")" && pwd)
ENGINE="$ROOT/engine/usr/bin"
CFG_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/openmw"
CFG="$CFG_DIR/openmw.cfg"
LOOK_FILE="$ROOT/look.txt"
TEXTURES_MARKER="$ROOT/pixelart/textures/isowind-generated.txt"

die() { echo "Isowind: $*" >&2; exit 1; }

setup_morrowind() {
    grep -qix 'content=Morrowind.esm' "$CFG" 2>/dev/null && return
    MW=""
    for d in "$MORROWIND" \
        "$HOME/.steam/steam/steamapps/common/Morrowind" \
        "$HOME/.local/share/Steam/steamapps/common/Morrowind" \
        "$HOME/.var/app/com.valvesoftware.Steam/.local/share/Steam/steamapps/common/Morrowind"; do
        if [ -n "$d" ] && [ -f "$d/Data Files/Morrowind.esm" ]; then MW=$d; break; fi
    done
    [ -n "$MW" ] || die "can't find Morrowind. Run again as: MORROWIND=/path/to/Morrowind $0
(the folder that has 'Data Files' and Morrowind.ini in it)"
    echo "Setting up OpenMW for Morrowind in $MW"
    mkdir -p "$CFG_DIR"
    printf 'data="%s"\n' "$MW/Data Files" >> "$CFG"
    # Plugins, archives and the game's settings from Morrowind.ini, as the wizard does
    "$ENGINE/openmw-iniimporter" -g -i "$MW/Morrowind.ini" -c "$CFG" -o "$CFG"
    grep -qix 'content=Morrowind.esm' "$CFG" || die "setting up from $MW/Morrowind.ini didn't work (see $CFG)"
}

build_textures() {
    [ -f "$TEXTURES_MARKER" ] && return
    # Python 3 with Pillow: the system's, or a private one in tools/venv (distros won't let pip install into theirs)
    PY=python3
    VENV="$ROOT/tools/venv"
    if ! python3 -c 'import PIL' 2>/dev/null; then
        PY="$VENV/bin/python"
        if ! "$PY" -c 'import PIL' 2>/dev/null; then
            echo "The pixel art look needs Pillow for Python 3; installing it into $VENV"
            { python3 -m venv "$VENV" && "$PY" -m pip install --quiet Pillow; } || die "couldn't install Pillow.
Install your distribution's python3-pillow (or python3-venv), or play with the normal look: $0 normal"
        fi
    fi
    DATA=$(sed -n 's/^data="\{0,1\}\(.*Data Files\)"\{0,1\}$/\1/p' "$CFG" | head -n 1)
    [ -n "$DATA" ] || die "no Morrowind 'Data Files' in $CFG"
    echo "Building the pixel art textures from your Morrowind files (only this once, a few minutes)..."
    "$PY" "$ROOT/tools/retro_textures.py" --data "$DATA" --out "$ROOT/pixelart"
    echo "Built by isowind.sh from your Morrowind files" > "$TEXTURES_MARKER"
}

play() {
    LOOK=${1:-$(cat "$LOOK_FILE" 2>/dev/null || echo pixelart)}
    case $LOOK in pixelart|normal) ;; *) die "unknown look '$LOOK' (pixelart or normal)" ;; esac
    echo "$LOOK" > "$LOOK_FILE"
    setup_morrowind
    [ "$LOOK" = pixelart ] && build_textures
    exec "$ENGINE/openmw" --config "$ROOT/profiles/$LOOK" $CONNECT
}

case $1 in
    host)
        cd "$ROOT/server"
        echo "Starting the server (Ctrl+C stops it). Play on it with: $0 join 127.0.0.1"
        echo "Friends join with this computer's address; the host's router needs UDP port 25565 forwarded to it."
        exec "$ENGINE/tes3mp-server" ;;
    join)
        [ -n "$2" ] || die "join needs the server's address, e.g. $0 join 203.0.113.7"
        CONNECT="--connect=$2"
        play "$3" ;;
    ""|pixelart|normal)
        play "$1" ;;
    *)
        sed -n '2,11p' "$0" | sed 's/^# \{0,1\}//'
        exit 1 ;;
esac
