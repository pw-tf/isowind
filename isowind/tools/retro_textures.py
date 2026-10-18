#!/usr/bin/env python3
"""Build low resolution, reduced colour copies of Morrowind's textures.

Reads every texture straight out of the BSA archives (no extraction), shrinks it
with a box filter, cuts it down to a small per-texture palette and writes it as an
uncompressed .dds into ../data/textures/. OpenMW always prefers a .dds over
the original name, and that data folder is the last data= line in openmw.cfg, so
these override the vanilla textures. Remove that data= line to undo.

UI art (menus, cursors, level-up pictures, birthsigns, map icons) and water are
left alone.

Requires Pillow:  python tools/retro_textures.py [--scale 2] [--colors 32]
"""

import argparse
import io
import re
import struct
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_OUT = ROOT / "data"
DEFAULT_DATA = Path(r"I:\SteamLibrary\steamapps\common\Morrowind\Data Files")
# Later archives override earlier ones, as with fallback-archive= in openmw.cfg.
ARCHIVES = ["Morrowind.bsa", "Tribunal.bsa", "Bloodmoon.bsa"]

SKIP = re.compile(
    r"^textures\\(levelup|birthsigns|magicitem|water|nvwater)\\"
    r"|^textures\\(menu_|tx_menu|cursor|compass|mapfow|target|scroll|bookicon|tx_ui_)"
    r"|_icon\.dds$",
    re.IGNORECASE,
)


def read_bsa(path):
    """Yield (name, bytes) for every file in a Morrowind (version 0x100) BSA."""
    data = path.read_bytes()
    version, hash_offset, count = struct.unpack_from("<3I", data, 0)
    if version != 0x100:
        raise ValueError(f"{path.name}: not a Morrowind BSA")
    records = struct.unpack_from(f"<{count * 2}I", data, 12)
    name_offsets = struct.unpack_from(f"<{count}I", data, 12 + count * 8)
    names_start = 12 + count * 12
    data_start = 12 + hash_offset + count * 8
    for i in range(count):
        start = names_start + name_offsets[i]
        name = data[start : data.index(b"\0", start)].decode("latin-1")
        size, offset = records[i * 2], records[i * 2 + 1]
        yield name, data[data_start + offset : data_start + offset + size]


def dds_bytes(image):
    """Uncompressed 32-bit BGRA .dds without mipmaps (OpenMW generates them)."""
    image = image.convert("RGBA")
    w, h = image.size
    DDSD_CAPS, DDSD_HEIGHT, DDSD_WIDTH, DDSD_PITCH, DDSD_PIXELFORMAT = 0x1, 0x2, 0x4, 0x8, 0x1000
    DDPF_ALPHAPIXELS, DDPF_RGB = 0x1, 0x40
    flags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT
    pixel_format = struct.pack(
        "<8I", 32, DDPF_RGB | DDPF_ALPHAPIXELS, 0, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000
    )
    header = struct.pack("<7I", 124, flags, h, w, w * 4, 0, 0) + b"\0" * 44 + pixel_format
    header += struct.pack("<5I", 0x1000, 0, 0, 0, 0)  # DDSCAPS_TEXTURE
    return b"DDS " + header + image.tobytes("raw", "BGRA")


def retro(image, scale, colors, min_size):
    image = image.convert("RGBA")
    has_alpha = image.getextrema()[3][0] < 255

    w, h = image.size
    f = scale
    while f > 1 and (w // f < min_size or h // f < min_size):
        f //= 2
    if f > 1:
        image = image.reduce(f)  # box filter

    rgb = image.convert("RGB")
    if colors:
        rgb = rgb.quantize(colors=colors, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE).convert("RGB")
    if has_alpha:
        rgb.putalpha(image.getchannel("A"))
    return rgb


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--data", type=Path, default=DEFAULT_DATA, help="Morrowind 'Data Files' folder")
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT, help="output data folder")
    parser.add_argument("--scale", type=int, default=2, choices=(1, 2, 4, 8),
                        help="divide texture size by this (2: 256 -> 128, 4: 256 -> 64, PS1-like)")
    parser.add_argument("--colors", type=int, default=32, help="colours per texture, 0 to keep all")
    parser.add_argument("--min-size", type=int, default=16, help="never shrink below this many pixels")
    args = parser.parse_args()

    textures = {}
    for archive in ARCHIVES:
        path = args.data / archive
        if not path.exists():
            print(f"skipping missing {path}", file=sys.stderr)
            continue
        for name, blob in read_bsa(path):
            if name.lower().startswith("textures\\"):
                textures[name.lower()] = blob

    done = skipped = failed = 0
    for name, blob in sorted(textures.items()):
        if SKIP.search(name):
            skipped += 1
            continue
        try:
            image = retro(Image.open(io.BytesIO(blob)), args.scale, args.colors, args.min_size)
        except Exception as e:  # a handful of odd formats are not worth failing the run for
            print(f"failed {name}: {e}", file=sys.stderr)
            failed += 1
            continue
        target = args.out / Path(*name.split("\\")).with_suffix(".dds")
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(dds_bytes(image))
        done += 1

    print(f"wrote {done} textures to {args.out / 'textures'} (skipped {skipped} UI/water, {failed} failed)",
          file=sys.stderr)


if __name__ == "__main__":
    main()
