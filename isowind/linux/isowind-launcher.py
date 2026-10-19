#!/usr/bin/env python3
"""Isowind's launcher window on Linux: Isowind.exe's window (Isowind.cs), the same layout, colours and words, in Tk.
isowind.sh gui opens it, as does isowind.sh started from a file manager. Playing and joining go through isowind.sh
beside it (it sets Morrowind up the first time and builds the pixel art textures); hosting writes the server's
settings and starts the server, as Isowind.exe's "Host a server" does."""

import glob
import os
import re
import shutil
import subprocess
import sys
import tkinter as tk
from tkinter import filedialog, messagebox, ttk
from tkinter import font as tkfont

ROOT = os.path.dirname(os.path.abspath(__file__))
SCRIPT = os.path.join(ROOT, 'isowind.sh')
ENGINE = os.path.join(ROOT, 'engine', 'usr', 'bin')
SERVER_DIR = os.path.join(ROOT, 'server')
SERVER = os.path.join(ENGINE, 'tes3mp-server')
LOOK_FILE = os.path.join(ROOT, 'look.txt')
MODE_FILE = os.path.join(ROOT, 'profiles', 'launcher-mode.txt')  # as Isowind.exe keeps it
ADDRESS_FILE = os.path.join(ROOT, 'profiles', 'launcher-server.txt')
MORROWIND_FILE = os.path.join(ROOT, 'morrowind-folder.txt')
LOG = os.path.join(ROOT, 'launcher.log')
USER_CFG = os.path.join(os.environ.get('XDG_CONFIG_HOME') or os.path.join(os.path.expanduser('~'), '.config'),
                        'openmw', 'openmw.cfg')
DEFAULT_PORT = 25565

# Isowind.exe's colours
BACK = '#181512'
PANEL = '#26211c'
GOLD = '#caa560'
SAND = '#dfc99f'
DIM = '#968769'
HOVER = '#3c3226'


def read_file(path, default=''):
    try:
        with open(path, encoding='utf-8') as f:
            return f.read().strip() or default
    except OSError:
        return default


def write_file(path, text):
    try:
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, 'w', encoding='utf-8') as f:
            f.write(text + '\n')
    except OSError:
        pass


def morrowind_configured():
    try:
        with open(USER_CFG, encoding='utf-8', errors='replace') as f:
            return any(line.strip().lower() == 'content=morrowind.esm' for line in f)
    except OSError:
        return False


def controller_plugged_in():
    return bool(glob.glob('/dev/input/js*'))


# "host", "host:port": the address and port to connect to, or None if it doesn't look like one (Isowind.cs)
def parse_server_address(text):
    text = (text or '').strip()
    if not text:
        return None
    address, port = text, DEFAULT_PORT
    if text.count(':') == 1:
        address, port_text = text.rsplit(':', 1)
        if not port_text.isdigit() or not 0 < int(port_text) <= 65535:
            return None
        port = int(port_text)
    if not address or ' ' in address:
        return None
    return f'{address}:{port}'


# The server's settings: tes3mp-server.cfg's over tes3mp-server-default.cfg's, as isowind.sh host keeps them
def server_setting(section, key, default):
    value = default
    for name in ('tes3mp-server-default.cfg', 'tes3mp-server.cfg'):
        path = os.path.join(SERVER_DIR, name)
        if not os.path.exists(path):
            continue
        current = None
        with open(path, encoding='utf-8', errors='replace') as f:
            for line in f:
                line = line.rstrip('\r\n')
                if line.startswith('['):
                    current = line.strip()
                elif current == f'[{section}]' and not line.startswith('#') and '=' in line:
                    k, v = line.split('=', 1)
                    if k.strip() == key:
                        value = v.strip()
    return value


def set_server_setting(section, key, value):
    path = os.path.join(SERVER_DIR, 'tes3mp-server.cfg')
    if os.path.exists(path):
        with open(path, encoding='utf-8', errors='replace') as f:
            lines = f.read().splitlines()
    else:
        lines = ["# This server's settings, over those in tes3mp-server-default.cfg (the launcher writes them)"]
    out, insection, done = [], False, False
    for line in lines:
        if line.startswith('['):
            if insection and not done:
                out.append(f'{key} = {value}')
                done = True
            insection = line.strip() == f'[{section}]'
        elif insection and not done and '=' in line and line.split('=', 1)[0].strip() == key:
            out.append(f'{key} = {value}')
            done = True
            continue
        out.append(line)
    if not done:
        if not insection:
            out += ['', f'[{section}]']
        out.append(f'{key} = {value}')
    with open(path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(out) + '\n')


def pick_font(candidates, size, weight='normal'):
    families = set(tkfont.families())
    for name in candidates:
        if name in families:
            return tkfont.Font(family=name, size=size, weight=weight)
    return tkfont.Font(size=size, weight=weight)


# The icon (make_icon.py): an isometric pixel-art tile, sand top and two shaded sides, outlined
def make_icon(size=64):
    s = size / 32
    top = [(16 * s, 3 * s), (29 * s, 10 * s), (16 * s, 17 * s), (3 * s, 10 * s)]
    left = [(3 * s, 10 * s), (16 * s, 17 * s), (16 * s, 29 * s), (3 * s, 22 * s)]
    right = [(29 * s, 10 * s), (16 * s, 17 * s), (16 * s, 29 * s), (29 * s, 22 * s)]

    def inside(poly, x, y):
        hit = False
        for (x1, y1), (x2, y2) in zip(poly, poly[1:] + poly[:1]):
            if (y1 > y) != (y2 > y) and x < x1 + (y - y1) * (x2 - x1) / (y2 - y1):
                hit = not hit
        return hit

    def near_edge(poly, x, y):
        for (x1, y1), (x2, y2) in zip(poly, poly[1:] + poly[:1]):
            dx, dy = x2 - x1, y2 - y1
            t = max(0.0, min(1.0, ((x - x1) * dx + (y - y1) * dy) / (dx * dx + dy * dy)))
            if ((x - x1 - t * dx) ** 2 + (y - y1 - t * dy) ** 2) ** 0.5 < s * 0.6:
                return True
        return False

    image = tk.PhotoImage(width=size, height=size)
    rows = []
    for py in range(size):
        row = []
        for px in range(size):
            x, y = px + 0.5, py + 0.5
            colour = None
            for poly, fill in ((top, '#d6b26e'), (left, '#785430'), (right, '#4e3622')):
                if inside(poly, x, y):
                    colour = '#3c2816' if near_edge(poly, x, y) else fill
                    break
            row.append(colour or BACK)
        rows.append('{' + ' '.join(row) + '}')
    image.put(' '.join(rows))
    # The background outside the tile is see-through
    for py in range(size):
        for px in range(size):
            if image.get(px, py) == (24, 21, 18):
                image.transparency_set(px, py, True)
    return image


CREDITS = (
    "Isowind is a modified OpenMW with a multiplayer port of TES3MP. It needs your own copy of The Elder Scrolls III: "
    "Morrowind (Bethesda Softworks); none of the game's files are included.\n\n"
    "ENGINE\n"
    "OpenMW - openmw.org - by the OpenMW team and contributors (licenses/OpenMW-AUTHORS.md). "
    "GNU General Public License v3 (licenses/GPL-3.0.txt). Isowind's changes are under the same license; "
    "the source is in the source folder.\n\n"
    "TES3MP - tes3mp.com - by David Cernat, Stanislav Zhukov and contributors. The multiplayer code in Isowind is "
    "ported from it, and the server runs its CoreScripts. GNU General Public License v3.\n\n"
    "BUNDLED MODS\n"
    "Rafael's Shader Pack - by Rafael, with Phal, Hrnchamd, Dexter and Wazabear. GNU General Public License v3 "
    "(mods/RafaelsShaderPack/LICENSE). Includes SMAA by Jorge Jimenez, Jose I. Echevarria, Belen Masia, "
    "Fernando Navarro and Diego Gutierrez, work by Fernando Garcia Linan, and NVIDIA code, under their notices in "
    "the shader files.\n\n"
    "Weapon Sheathing 1.6 - by Greatness7; OpenMW implementation by akortunov and the OpenMW team. Meshes by "
    "akortunov, Greatness7, Heinrich, London Rook, Lord Berandas, Melchior Dahrk, MementoMoritius, Petethegoat, "
    "PikachunoTM and Remiros. Free to use with credit to the authors and not for a fee "
    "(mods/WeaponSheathing/WeaponSheathing.txt).\n\n"
    "Questman - by fixer_m (Nexus Mods, Morrowind mod 59493). A quest log; press X in game (D-pad right on a "
    "controller).\n\n"
    "No Flickering - by Batkan1234. Steady lights instead of flickering ones (mods/NoFlickering).\n\n"
    "Morrowind Optimization Patch - by Remiros and Greatness7, with Axeljk, Borok, Half11, Hemaris, Lamb Shark, "
    "Melchior Dahrk, Nich, Revenorror, Sophie and Vegetto. Faster, fixed meshes "
    "(mods/MorrowindOptimizationPatch).\n\n"
    "Glow in the Dahrk 3.3.0 - by Melchior Dahrk and NullCascade, with Greatness7, Siberian Crab, Hrnchamd, "
    "Wolli and Atrayonis. Windows that glow at night (mods/GlowInTheDahrk).\n\n"
    "Graphic Herbalism 1.04 (MWSE and OpenMW edition) - by Greatness7 and Stuporstar. Plants show as picked "
    "(mods/GraphicHerbalism).\n\n"
    "Project Atlas 0.7.5 - by FloorBelow, Greatness7, Lord Berandas, Melchior Dahrk, MwGek, Petethegoat, "
    "Pop000100, R-Zero, Remiros, revenorror, RubberMan, Sataniel, Stuporstar, vtastek and Wollibeebee. "
    "Texture atlases for faster drawing (mods/ProjectAtlas).\n\n"
    "Expansion Delay 1.3 - by Half11. Tribunal's assassins and Bloodmoon's rumours wait until later "
    "(mods/ExpansionDelay).\n\n"
    "LIBRARIES (used by OpenMW)\n"
    "OpenSceneGraph (OpenSceneGraph Public License), Bullet Physics (zlib), MyGUI (MIT), SDL 2 and SDL_image "
    "(zlib), OpenAL Soft (LGPL 2.1), FFmpeg (LGPL 2.1), LuaJIT (MIT), RakNet (BSD), Recast & Detour (zlib), "
    "SQLite (public domain), yaml-cpp (MIT), ICU (Unicode License), Boost (Boost Software License), FreeType "
    "(FreeType License), zlib, libpng, libjpeg-turbo, LibTIFF, libxml2, Collada DOM, fmt (MIT), Brotli, zstd, "
    "XZ Utils, bzip2, libiconv and uriparser, each under its own license.")


class Styled:
    """Isowind.exe's widgets: flat buttons with a gold border that light up under the mouse, panels, labels.
    The fonts are the serifs Isowind.exe shows under Wine (Segoe UI isn't there), so the two windows look alike."""

    def __init__(self):
        self.text_font = pick_font(['Liberation Serif', 'DejaVu Serif', 'Noto Serif', 'Segoe UI', 'Noto Sans', 'DejaVu Sans'], 10)
        self.small_font = pick_font(['Liberation Serif', 'DejaVu Serif', 'Noto Serif', 'Segoe UI', 'Noto Sans', 'DejaVu Sans'], 9)
        self.tiny_font = pick_font(['Liberation Serif', 'DejaVu Serif', 'Noto Serif', 'Segoe UI', 'Noto Sans', 'DejaVu Sans'], 8)
        self.title_font = pick_font(['Georgia', 'Gelasio', 'Liberation Serif', 'DejaVu Serif', 'Noto Serif'], 30, 'bold')
        self.play_font = pick_font(['Liberation Serif', 'DejaVu Serif', 'Noto Serif', 'Segoe UI', 'Noto Sans', 'DejaVu Sans'], 16, 'bold')

    def button(self, parent, text, x, y, w, h, command, font=None):
        button = tk.Button(parent, text=text, command=command, font=font or self.text_font, bg=PANEL, fg=SAND,
                           activebackground=HOVER, activeforeground=SAND, relief='flat', bd=0,
                           highlightthickness=1, highlightbackground=GOLD, highlightcolor=GOLD, cursor='hand2',
                           disabledforeground=DIM)
        button.bind('<Enter>', lambda e: button.configure(bg=HOVER) if button['state'] == 'normal' else None)
        button.bind('<Leave>', lambda e: button.configure(bg=PANEL))
        button.place(x=x, y=y, width=w, height=h)
        return button

    def label(self, parent, text, x, y, w, h, fg=SAND, font=None, anchor='w', justify='left'):
        label = tk.Label(parent, text=text, fg=fg, bg=parent['bg'], font=font or self.text_font, anchor=anchor,
                         justify=justify)
        label.place(x=x, y=y, width=w, height=h)
        return label

    def panel(self, parent, x, y, w, h):
        panel = tk.Frame(parent, bg=PANEL)
        panel.place(x=x, y=y, width=w, height=h)
        return panel

    def radio(self, parent, text, variable, value, x, y, w, h):
        radio = tk.Radiobutton(parent, text=text, variable=variable, value=value, bg=PANEL, fg=SAND,
                               selectcolor=BACK, activebackground=PANEL, activeforeground=SAND,
                               highlightthickness=0, font=self.text_font, anchor='w', cursor='hand2')
        radio.place(x=x, y=y, width=w, height=h)
        return radio

    def check(self, parent, text, variable, x, y, w, h):
        check = tk.Checkbutton(parent, text=text, variable=variable, bg=PANEL, fg=SAND, selectcolor=BACK,
                               activebackground=PANEL, activeforeground=SAND, highlightthickness=0,
                               font=self.text_font, anchor='w', cursor='hand2')
        check.place(x=x, y=y, width=w, height=h)
        return check

    def entry(self, parent, variable, x, y, w, h, show=None):
        entry = tk.Entry(parent, textvariable=variable, bg=BACK, fg=SAND, insertbackground=SAND, relief='flat',
                         highlightthickness=1, highlightbackground=DIM, highlightcolor=GOLD, font=self.text_font,
                         show=show)
        entry.place(x=x, y=y, width=w, height=h)
        return entry


def centre(window, width, height):
    window.update_idletasks()
    x = (window.winfo_screenwidth() - width) // 2
    y = (window.winfo_screenheight() - height) // 2
    window.geometry(f'{width}x{height}+{max(0, x)}+{max(0, y)}')


class Launcher(tk.Tk, Styled):
    WIDTH, HEIGHT = 460, 558  # Isowind.exe's 460 x 530, plus the controller line in the Look panel

    def __init__(self):
        tk.Tk.__init__(self)
        Styled.__init__(self)
        self.title('Isowind')
        self.configure(bg=BACK)
        self.resizable(False, False)
        centre(self, self.WIDTH, self.HEIGHT)
        try:
            self.icon = make_icon()
            self.wm_iconphoto(True, self.icon)
        except tk.TclError:
            pass

        self.look = tk.StringVar(value='normal' if read_file(MODE_FILE) == 'normal' else 'pixelart')
        self.controller = tk.BooleanVar(value=controller_plugged_in())
        self.address = tk.StringVar(value=read_file(ADDRESS_FILE))
        self.status = tk.StringVar(value='')

        self.label(self, 'ISOWIND', 0, 18, 460, 54, fg=GOLD, font=self.title_font, anchor='center')
        self.label(self, 'Morrowind, seen from above', 0, 72, 460, 22, fg=DIM, anchor='center')

        look = self.panel(self, 30, 110, 400, 146)
        self.label(look, 'Look', 14, 8, 300, 22, fg=GOLD)
        self.pixel_art = self.radio(look, 'Pixel art  -  low resolution, retro textures and lighting', self.look,
                                    'pixelart', 14, 36, 380, 26)
        self.normal = self.radio(look, "Normal  -  Morrowind's usual graphics", self.look, 'normal', 14, 66, 380, 26)
        self.check(look, 'Controller  -  menus made for a controller rather than a mouse', self.controller,
                   14, 96, 380, 26)
        self.label(look, 'The top-down camera works in both. The zoom (or F7) in game switches camera.', 14, 122,
                   380, 18, fg=DIM, font=self.tiny_font)

        self.play = self.button(self, 'Play', 30, 272, 400, 52, self.on_play, font=self.play_font)

        multiplayer = self.panel(self, 30, 336, 400, 122)
        self.label(multiplayer, 'Multiplayer', 14, 8, 300, 22, fg=GOLD)
        self.label(multiplayer, 'Server address', 14, 38, 110, 22)
        entry = self.entry(multiplayer, self.address, 126, 37, 160, 24)
        entry.bind('<Return>', lambda e: self.on_join())
        self.join = self.button(multiplayer, 'Join', 296, 34, 90, 30, self.on_join)
        self.host = self.button(multiplayer, 'Host a server', 14, 74, 140, 30, self.on_host, font=self.small_font)
        self.label(multiplayer, f'Friends need UDP port {DEFAULT_PORT}\nforwarded to you, or a shared VPN', 162, 72,
                   230, 34, fg=DIM, font=self.tiny_font)

        self.morrowind = self.button(self, 'Morrowind folder  (where Morrowind is installed)', 30, 470, 300, 34,
                                     self.on_morrowind_folder, font=self.small_font)
        self.button(self, 'Licenses', 340, 470, 90, 34, self.show_licenses, font=self.small_font)

        style = ttk.Style(self)
        style.theme_use('default')
        style.configure('Isowind.Horizontal.TProgressbar', troughcolor=PANEL, background=GOLD, bordercolor=PANEL,
                        lightcolor=GOLD, darkcolor=GOLD)
        self.progress = ttk.Progressbar(self, style='Isowind.Horizontal.TProgressbar', mode='indeterminate')
        self.label(self, '', 30, 528, 400, 22, fg=DIM, font=self.small_font).configure(textvariable=self.status)

        if not os.path.exists(SERVER) or not os.path.isdir(SERVER_DIR):
            self.host.configure(state='disabled')
        if not os.path.exists(os.path.join(ENGINE, 'openmw')):
            self.status.set('engine/usr/bin/openmw is missing - unpack the whole Isowind folder.')
        elif not morrowind_configured():
            self.status.set('First time? Press Play to set up your Morrowind installation.')

    # ---- playing -----------------------------------------------------------------------------------------------

    def launch(self, arguments):
        look = self.look.get()
        write_file(MODE_FILE, look)
        write_file(LOOK_FILE, look)
        env = dict(os.environ)
        folder = read_file(MORROWIND_FILE)
        if folder:
            env['MORROWIND'] = folder
        args = [SCRIPT] + arguments + [look, '--controller' if self.controller.get() else '--no-controller']
        try:
            log = open(LOG, 'w', encoding='utf-8')
            process = subprocess.Popen(args, stdout=log, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL,
                                       cwd=ROOT, env=env)
        except OSError as e:
            messagebox.showerror('Isowind', f"Could not start {os.path.basename(SCRIPT)}:\n{e}", parent=self)
            return
        for widget in (self.play, self.join, self.host, self.pixel_art, self.normal, self.morrowind):
            widget.configure(state='disabled')
        self.progress.place(x=30, y=512, width=400, height=12)
        self.progress.start(12)
        first = not morrowind_configured()
        textures = look == 'pixelart' and not os.path.exists(os.path.join(ROOT, 'pixelart', 'textures',
                                                                          'isowind-generated.txt'))
        self.status.set('Setting up your Morrowind installation...' if first
                        else 'Building the pixel art textures from your Morrowind files (a few minutes)...' if textures
                        else 'Starting...')
        self.after(1000, lambda: self.watch(process, 0))

    # Once the game is on its way the window closes, as Isowind.exe does; a quick failure is shown instead
    def watch(self, process, seconds):
        code = process.poll()
        if code is None:
            if seconds >= 10 and self.status.get() == 'Starting...':
                self.destroy()
                return
            if seconds >= 10 and os.path.exists(os.path.join(ROOT, 'pixelart', 'textures', 'isowind-generated.txt')) \
                    and morrowind_configured():
                self.destroy()
                return
            self.after(1000, lambda: self.watch(process, seconds + 1))
            return
        self.progress.stop()
        self.progress.place_forget()
        for widget in (self.play, self.join, self.host, self.pixel_art, self.normal, self.morrowind):
            widget.configure(state='normal')
        if not os.path.exists(SERVER):
            self.host.configure(state='disabled')
        if code == 0:
            self.destroy()
            return
        self.status.set('')
        output = read_file(LOG, 'It stopped without saying why.')
        messagebox.showerror('Isowind', output[-1500:], parent=self)

    def on_play(self):
        self.launch([])

    def on_join(self):
        address = parse_server_address(self.address.get())
        if address is None:
            messagebox.showinfo('Isowind - multiplayer',
                                'Type the address of the server to join: its IP address (like 203.0.113.7) or name, '
                                f'optionally with a port (like 203.0.113.7:{DEFAULT_PORT}).\n\n'
                                'To join a server on this computer, use 127.0.0.1.', parent=self)
            return
        write_file(ADDRESS_FILE, self.address.get().strip())
        self.launch(['join', address])

    # The Morrowind folder, for when it isn't where isowind.sh looks (Steam's usual places)
    def on_morrowind_folder(self):
        folder = filedialog.askdirectory(parent=self, title='The folder Morrowind is installed in (with "Data Files")',
                                         initialdir=read_file(MORROWIND_FILE) or os.path.expanduser('~'))
        if not folder:
            return
        if not os.path.exists(os.path.join(folder, 'Data Files', 'Morrowind.esm')):
            messagebox.showinfo('Isowind', 'That folder has no "Data Files/Morrowind.esm" in it. Pick the folder '
                                'Morrowind is installed in (the one with Morrowind.ini and "Data Files").',
                                parent=self)
            return
        write_file(MORROWIND_FILE, folder)
        self.status.set('Morrowind folder set. Press Play to set it up.' if not morrowind_configured()
                        else 'Morrowind folder set.')

    # ---- hosting -----------------------------------------------------------------------------------------------

    def on_host(self):
        if subprocess.run(['pgrep', '-x', 'tes3mp-server'], capture_output=True).returncode == 0:
            messagebox.showinfo('Isowind - multiplayer', 'A server is already running on this computer.',
                                parent=self)
            return
        HostDialog(self)

    def server_started(self, port, how):
        self.address.set('127.0.0.1' if port == DEFAULT_PORT else f'127.0.0.1:{port}')
        self.status.set('Server started. Press Join to play on it.')
        messagebox.showinfo('Isowind - hosting',
                            f'The server is running {how}.\n\n'
                            'To play on it yourself, press Join (the address is set to 127.0.0.1, this computer).\n\n'
                            'Friends join with this computer\'s internet address (search "what is my IP"), which '
                            f'needs UDP port {port} forwarded to this computer in your router. Or all of you join '
                            'the same VPN (e.g. Tailscale, ZeroTier, Radmin VPN) and friends use this computer\'s '
                            'VPN address.\n\n'
                            'Everyone needs Morrowind with Tribunal and Bloodmoon, and no other mods turned on.',
                            parent=self)

    # ---- licenses ----------------------------------------------------------------------------------------------

    def show_licenses(self):
        dialog = tk.Toplevel(self)
        dialog.title('Isowind - Licenses')
        dialog.configure(bg=BACK)
        dialog.resizable(False, False)
        dialog.transient(self)
        centre(dialog, 560, 520)
        text = tk.Text(dialog, wrap='word', bg=PANEL, fg=SAND, relief='flat', bd=0, font=self.small_font,
                       padx=8, pady=8, highlightthickness=0)
        text.insert('1.0', CREDITS)
        text.configure(state='disabled')
        text.place(x=16, y=16, width=510, height=438)
        scroll = tk.Scrollbar(dialog, command=text.yview, bg=PANEL, troughcolor=BACK, activebackground=HOVER,
                              highlightthickness=0, bd=0)
        scroll.place(x=526, y=16, width=18, height=438)
        text.configure(yscrollcommand=scroll.set)
        self.button(dialog, 'Open the licenses folder', 16, 468, 220, 34,
                    lambda: subprocess.Popen(['xdg-open', os.path.join(ROOT, 'licenses')]), font=self.small_font)
        self.button(dialog, 'Close', 444, 468, 100, 34, dialog.destroy, font=self.small_font)


class HostDialog(tk.Toplevel, Styled):
    """The server's settings, as Isowind.exe's "Host a server" asks them (and isowind.sh host), then the server in a
    terminal window, or in the background with its output in server/launcher-server.log"""

    def __init__(self, parent):
        tk.Toplevel.__init__(self, parent)
        Styled.__init__(self)
        self.parent = parent
        self.title('Isowind - Host a server')
        self.configure(bg=BACK)
        self.resizable(False, False)
        self.transient(parent)
        centre(self, 460, 468)

        self.label(self, 'Host a server', 0, 14, 460, 30, fg=GOLD, font=self.play_font, anchor='center')
        panel = self.panel(self, 30, 56, 400, 332)
        self.fields = {}
        rows = [
            ('Server name', 'General', 'hostname', 'Isowind server', None),
            ('Port', 'General', 'port', str(DEFAULT_PORT), None),
            ('Most players', 'General', 'maximumPlayers', '16', None),
            ('Password (empty: anyone)', 'General', 'password', '', None),
            ('Game', 'Isowind', 'mode', 'coop', ['coop', 'mmo']),
            ('Difficulty, -100 to 100', 'Isowind', 'difficulty', '0', None),
            ('Enemy respawns (MMO), x', 'Isowind', 'respawnSpeed', '1', ['0.5', '1', '2', '4']),
            ('Time of day, x faster', 'Isowind', 'timeSpeed', '1', ['0.5', '1', '2', '4']),
        ]
        y = 12
        for label, section, key, default, choices in rows:
            self.label(panel, label, 14, y, 190, 26)
            var = tk.StringVar(value=server_setting(section, key, default))
            if choices:
                menu = tk.OptionMenu(panel, var, *choices)
                menu.configure(bg=BACK, fg=SAND, activebackground=HOVER, activeforeground=SAND, relief='flat',
                               highlightthickness=1, highlightbackground=DIM, font=self.text_font, bd=0,
                               indicatoron=False, cursor='hand2')
                menu['menu'].configure(bg=PANEL, fg=SAND, activebackground=HOVER, activeforeground=SAND)
                menu.place(x=214, y=y, width=170, height=26)
            else:
                self.entry(panel, var, 214, y, 170, 26)
            self.fields[(section, key)] = var
            y += 32
        self.pvp = tk.BooleanVar(value=server_setting('Isowind', 'pvp', 'false') == 'true')
        self.check(panel, 'Players can hurt each other (PvP)', self.pvp, 14, y, 370, 26)
        y += 30
        self.label(panel, 'Co-op: one shared world, as in single player. MMO: everyone loots and quests for\n'
                   'themselves, and enemies come back. Keep the same game for a world.', 14, y, 370, 34, fg=DIM,
                   font=self.tiny_font)

        self.button(self, 'Start the server', 30, 404, 180, 40, self.start, font=self.text_font)
        self.button(self, 'Cancel', 330, 404, 100, 40, self.destroy, font=self.text_font)

    def start(self):
        values = {key: var.get().strip() for key, var in self.fields.items()}
        port = values[('General', 'port')]
        if not port.isdigit() or not 1024 <= int(port) <= 65535:
            messagebox.showinfo('Isowind', 'The port is a number from 1024 to 65535.', parent=self)
            return
        players = values[('General', 'maximumPlayers')]
        if not players.isdigit() or not 1 <= int(players) <= 64:
            messagebox.showinfo('Isowind', 'Most players is a number from 1 to 64.', parent=self)
            return
        difficulty = values[('Isowind', 'difficulty')]
        if not re.fullmatch(r'-?\d{1,3}', difficulty) or not -100 <= int(difficulty) <= 100:
            messagebox.showinfo('Isowind', 'Difficulty is a number from -100 to 100.', parent=self)
            return
        if not os.path.exists(SERVER):
            messagebox.showerror('Isowind', f'No server here: {SERVER}', parent=self)
            return
        for (section, key), value in values.items():
            set_server_setting(section, key, value)
        set_server_setting('Isowind', 'pvp', 'true' if self.pvp.get() else 'false')

        terminals = [('x-terminal-emulator', ['-e']), ('foot', []), ('alacritty', ['-e']), ('kitty', []),
                     ('ghostty', ['-e']), ('gnome-terminal', ['--']), ('konsole', ['-e']),
                     ('xfce4-terminal', ['-x']), ('xterm', ['-e'])]
        command = None
        for name, option in terminals:
            path = shutil.which(name)
            if path:
                command = [path] + option + [SERVER]
                break
        try:
            if command:
                subprocess.Popen(command, cwd=SERVER_DIR)
                how = 'in its own window; close that window to stop it'
            else:
                log = open(os.path.join(SERVER_DIR, 'launcher-server.log'), 'w', encoding='utf-8')
                subprocess.Popen([SERVER], cwd=SERVER_DIR, stdout=log, stderr=subprocess.STDOUT,
                                 stdin=subprocess.DEVNULL, start_new_session=True)
                how = 'in the background (its output is in server/launcher-server.log); stop it from a task manager'
        except OSError as e:
            messagebox.showerror('Isowind', f"Could not start the server: {e}", parent=self)
            return
        self.destroy()
        self.parent.server_started(int(port), how)


if __name__ == '__main__':
    if not os.path.exists(SCRIPT):
        print(f'isowind-launcher.py: no isowind.sh beside it in {ROOT}', file=sys.stderr)
        sys.exit(1)
    Launcher().mainloop()
