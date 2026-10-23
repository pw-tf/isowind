#!/bin/sh
# Isowind on Linux: what Isowind.exe does on Windows.
#
#   ./isowind.sh                  play (pixel art look, or the look you used last)
#   ./isowind.sh normal           play with the normal look (or: pixelart)
#   ./isowind.sh join ADDRESS     play on a server, e.g. ./isowind.sh join 203.0.113.7:25565 (look optional after it)
#   ./isowind.sh host             run a server here, in this terminal, after asking its settings (others join
#                                 with this computer's address)
#   ./isowind.sh gui              the launcher window (look, controller, join, host), like Isowind.exe; also what
#                                 isowind.sh opens when started from a file manager
#   --controller, --no-controller menus made for a controller, or Morrowind's own, with any of the above; otherwise
#                                 whichever fits what is plugged in (a controller that comes with a stream, as
#                                 Moonlight's does, wants --controller)
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

# set_setting FILE SECTION KEY VALUE: sets KEY in [SECTION] of a settings file (OpenMW's settings.cfg, the server's
# tes3mp-server.cfg); a new one goes after the section's last setting
set_setting() {
    awk -v section="[$2]" -v key="$3" -v value="$4" '
        function emit() { print key " = " value; done = 1 }
        function flush() { printf "%s", blanks; blanks = "" }
        $0 == section { print; insection = 1; next }
        insection && /^[ \t\r]*$/ { blanks = blanks $0 "\n"; next }
        /^\[/ && insection { if (!done) emit(); insection = 0 }
        insection && index($0, key " =") == 1 { flush(); emit(); next }
        { flush(); print }
        END { if (!done) { if (!insection) print "\n" section; emit() } flush() }
    ' "$1" > "$1.tmp" && mv "$1.tmp" "$1"
}

# get_server_setting SECTION KEY DEFAULT: from the server's settings files (in the current folder), tes3mp-server.cfg's
# over tes3mp-server-default.cfg's
get_server_setting() {
    VALUE=$(cat tes3mp-server-default.cfg tes3mp-server.cfg 2>/dev/null | tr -d '\r' | awk -v section="[$1]" -v key="$2" '
        /^\[/ { insection = ($0 == section); next }
        insection && !/^#/ && (i = index($0, "=")) {
            k = substr($0, 1, i - 1); v = substr($0, i + 1)
            gsub(/^[ \t]+|[ \t]+$/, "", k); gsub(/^[ \t]+|[ \t]+$/, "", v)
            if (k == key) found = v
        }
        END { print found }')
    echo "${VALUE:-$3}"
}

# ask QUESTION DEFAULT [PATTERN]: the answer in REPLY (Enter keeps the default); asked again until it matches the
# extended regular expression PATTERN
ask() {
    while :; do
        printf '%s [%s]: ' "$1" "$2"
        read -r REPLY || REPLY=""
        [ -n "$REPLY" ] || REPLY=$2
        if [ -z "$3" ] || printf '%s\n' "$REPLY" | grep -Eqx "$3"; then return; fi
        echo "  That isn't one of the choices."
    done
}

# The host's choices, kept in server/tes3mp-server.cfg (over tes3mp-server-default.cfg), as Isowind.exe's
# "Host a server" window does on Windows
ask_host_settings() {
    FILE=tes3mp-server.cfg
    [ -f "$FILE" ] || echo "# This server's settings, over those in tes3mp-server-default.cfg (isowind.sh host writes them)" > "$FILE"
    if [ ! -t 0 ]; then return; fi
    echo "Hosting a server. Press Enter to keep the choice in [brackets]."
    ask "Server name" "$(get_server_setting General hostname 'Isowind server')" '[^=#]{1,60}'
    set_setting "$FILE" General hostname "$REPLY"
    ask "Port" "$(get_server_setting General port 25565)" '[0-9]{4,5}'
    PORT=$REPLY
    set_setting "$FILE" General port "$REPLY"
    ask "Most players" "$(get_server_setting General maximumPlayers 16)" '[1-9]|[1-5][0-9]|6[0-4]'
    set_setting "$FILE" General maximumPlayers "$REPLY"
    PASSWORD=$(get_server_setting General password '')
    ask "Password (a single - for none)" "${PASSWORD:--}" '[^ ]{1,40}'
    [ "$REPLY" = "-" ] && REPLY=""
    set_setting "$FILE" General password "$REPLY"
    echo "Game: coop  - one shared world, as in single player"
    echo "      mmo   - everyone loots and quests for themselves, and enemies come back"
    ask "Game (coop or mmo)" "$(get_server_setting Isowind mode coop)" 'coop|mmo'
    MODE=$REPLY
    set_setting "$FILE" Isowind mode "$REPLY"
    PVP=$(get_server_setting Isowind pvp false)
    [ "$PVP" = true ] && PVP=yes || PVP=no
    ask "Can players hurt each other (PvP)? (yes or no)" "$PVP" 'yes|no'
    [ "$REPLY" = yes ] && REPLY=true || REPLY=false
    set_setting "$FILE" Isowind pvp "$REPLY"
    ask "Difficulty, -100 (very easy) to 100 (very hard), 0 is Morrowind's normal" \
        "$(get_server_setting Isowind difficulty 0)" '-?[0-9]{1,3}'
    set_setting "$FILE" Isowind difficulty "$REPLY"
    if [ "$MODE" = mmo ]; then
        ask "How fast enemies come back: 0.5, 1, 2 or 4 (1: 1.5 minutes for the weakest, 20 for bosses)" \
            "$(get_server_setting Isowind respawnSpeed 1)" '0\.5|1|2|4'
        set_setting "$FILE" Isowind respawnSpeed "$REPLY"
    fi
    ask "How fast the time of day goes: 0.5, 1, 2 or 4 (1: Morrowind's, an hour in 2 minutes)" \
        "$(get_server_setting Isowind timeSpeed 1)" '0\.5|1|2|4'
    set_setting "$FILE" Isowind timeSpeed "$REPLY"
}

play() {
    LOOK=${1:-$(cat "$LOOK_FILE" 2>/dev/null || echo pixelart)}
    case $LOOK in pixelart|normal) ;; *) die "unknown look '$LOOK' (pixelart or normal)" ;; esac
    echo "$LOOK" > "$LOOK_FILE"
    setup_morrowind
    [ "$LOOK" = pixelart ] && build_textures
    # Menus made for a controller when one is plugged in (or asked for with --controller), Morrowind's own (drag and
    # drop) otherwise
    if [ -z "$MENUS" ]; then
        if ls /dev/input/js* >/dev/null 2>&1; then MENUS=true; else MENUS=false; fi
    fi
    set_setting "$ROOT/profiles/$LOOK/settings.cfg" GUI "controller menus" "$MENUS"
    # Xbox controllers through the kernel's driver, not SDL's own: SDL's skips one it can't open (a Bluetooth one,
    # whose hidraw only root reads) when it has another with the same ids (Sunshine's virtual one, left after a stream)
    export SDL_JOYSTICK_HIDAPI_XBOX=${SDL_JOYSTICK_HIDAPI_XBOX:-0}
    exec "$ENGINE/openmw" --config "$ROOT/profiles/$LOOK" $CONNECT
}

launcher() {
    [ -f "$ROOT/isowind-launcher.py" ] || die "no launcher window here (isowind-launcher.py); use the commands above"
    python3 -c 'import tkinter' 2>/dev/null \
        || die "the launcher window needs Python 3's Tk: install python3-tk (Debian, Ubuntu) or tk (Arch, Fedora), or use the commands above"
    exec python3 "$ROOT/isowind-launcher.py"
}

# --controller / --no-controller, anywhere among the arguments (none of the others has spaces in it)
MENUS=""
ARGS=""
for arg in "$@"; do
    case $arg in
        --controller) MENUS=true ;;
        --no-controller) MENUS=false ;;
        *) ARGS="$ARGS $arg" ;;
    esac
done
# shellcheck disable=SC2086
set -- $ARGS

case $1 in
    host)
        cd "$ROOT/server"
        ask_host_settings
        PORT=$(get_server_setting General port 25565)
        echo "Starting the server (Ctrl+C stops it). Play on it with: $0 join 127.0.0.1:$PORT"
        echo "Friends join with this computer's address; the host's router needs UDP port $PORT forwarded to it."
        exec "$ENGINE/tes3mp-server" ;;
    join)
        [ -n "$2" ] || die "join needs the server's address, e.g. $0 join 203.0.113.7"
        CONNECT="--connect=$2"
        play "$3" ;;
    gui)
        launcher ;;
    "")
        # Started from a file manager (no terminal): the launcher window
        if [ ! -t 0 ] && [ -f "$ROOT/isowind-launcher.py" ]; then launcher; fi
        play "" ;;
    pixelart|normal)
        play "$1" ;;
    *)
        sed -n '2,13p' "$0" | sed 's/^# \{0,1\}//'
        exit 1 ;;
esac
