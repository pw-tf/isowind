#!/usr/bin/env python3
"""Makes an Isowind package folder one without the bundled third-party mods: OpenMW's own look with Isowind's on top,
for a public release (the mods' licences don't all allow passing them on). Run on the package, after it's assembled:

    strip_mods.py PACKAGE_DIR

make_package.ps1 -NoMods and package.sh (when it's given no mods) do. It takes out the mods folder; from both looks'
profiles the mods' data folders and plugins, the shader pack's post-processing and settings and the settings that
lean on them; from the server the mods' plugins it requires of players; and from the readme the list of mods. Each
change has to find what it changes, so the script stops (and says what) rather than leave a mod half in when the
profiles change."""

import os
import re
import shutil
import sys

# What a look's profile has for the mods, and what it has instead
PROFILE_SETTINGS = [
    # Rafael's Shader Pack: no post-processing but Isowind's own selection ring, which Lua adds
    (r"^# Rafael's Shader Pack:.*\n(?:#.*\n)*",
     "# No post-processing shaders but Isowind's top-down selection ring, which Lua adds\n"),
    (r"^chain = (?:HBAO|VAIO).*\n", "chain =\n"),
    # (the pack tonemapped; OpenMW's gamma is 1.0 anyway)
    (r"^# The shader pack tonemaps.*\n", ""),
    # Weapon Sheathing's meshes aren't there: OpenMW's default
    (r"^# Weapon Sheathing:.*\nweapon sheathing = true\n", ""),
    # Water reflections were left to the pack's wet world: OpenMW's default
    (r"^# Reflect only the sky:.*\nreflection detail = 0\n", ""),
]

README_MODS = (r"^Included mods \(in the mods folder.*?the profiles load them\.\n",
               "Isowind comes without other mods: add the ones you like in the OpenMW Launcher. A multiplayer server\n"
               "lets in only players whose plugins match its own (server\\CoreScripts\\data\\requiredDataFiles.json).\n")


def fail(message):
    print(f'strip_mods.py: {message}', file=sys.stderr)
    sys.exit(1)


def read(path):
    with open(path, encoding='utf-8', newline='') as f:
        text = f.read()
    return text.replace('\r\n', '\n'), '\r\n' in text


def write(path, text, crlf):
    with open(path, 'w', encoding='utf-8', newline='') as f:
        f.write(text.replace('\n', '\r\n') if crlf else text)


def replace(path, pattern, replacement, flags=re.MULTILINE):
    text, crlf = read(path)
    text, count = re.subn(pattern, lambda match: replacement, text, flags=flags)
    if count == 0:
        fail(f"{path} has no {pattern!r} to change (did it change?)")
    write(path, text, crlf)


def strip_profile(profile):
    # openmw.cfg: Isowind's own data folders and scripts only
    cfg = os.path.join(profile, 'openmw.cfg')
    text, crlf = read(cfg)
    kept, skipping_comment, dropped = [], False, 0
    for line in text.split('\n'):
        if line.startswith('# Bundled mods'):
            skipping_comment = True
            continue
        if skipping_comment and line.startswith('#'):
            continue
        skipping_comment = False
        if line.startswith('data="../../mods/') or (line.startswith('content=') and
                                                     line != 'content=isowind.omwscripts'):
            dropped += 1
            continue
        kept.append(line)
    if dropped == 0:
        fail(f'{cfg} has no mods in it to take out (did it change?)')
    write(cfg, '\n'.join(kept), crlf)

    settings = os.path.join(profile, 'settings.cfg')
    for pattern, replacement in PROFILE_SETTINGS:
        replace(settings, pattern, replacement)
    text, _ = read(settings)
    if re.search(r'shader pack|sheathing', text, re.IGNORECASE):
        fail(f'{settings} still mentions a mod after the changes')

    # The shader pack's own settings
    yaml = os.path.join(profile, 'shaders.yaml')
    if os.path.exists(yaml):
        os.remove(yaml)


def strip_server(package):
    required = os.path.join(package, 'server', 'CoreScripts', 'data', 'requiredDataFiles.json')
    # The note on the plugins Isowind ships, then each plugin after Bloodmoon's (with the comma before it)
    replace(required, r"^// Isowind also ships plugins.*\n(?://.*\n)*", '')
    replace(required, r',\n\s*\{"(?!Morrowind\.esm|Tribunal\.esm|Bloodmoon\.esm)[^"]+": \[[^\]]*\]\}', '')
    text, _ = read(required)
    if re.search(r'\.esp"', text, re.IGNORECASE):
        fail(f'{required} still requires a plugin after the changes')


def main():
    if len(sys.argv) != 2:
        fail('give the package folder (the one with profiles, server and README.txt in it)')
    package = sys.argv[1]
    if not os.path.isfile(os.path.join(package, 'README.txt')):
        fail(f'{package} is not an Isowind package (no README.txt)')

    mods = os.path.join(package, 'mods')
    if os.path.isdir(mods):
        shutil.rmtree(mods)
    for look in ('pixelart', 'normal'):
        strip_profile(os.path.join(package, 'profiles', look))
    strip_server(package)
    replace(os.path.join(package, 'README.txt'), README_MODS[0], README_MODS[1], re.MULTILINE | re.DOTALL)
    print(f'{package}: without the bundled mods')


if __name__ == '__main__':
    main()
