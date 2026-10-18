#!/usr/bin/env python3
"""Build data/textures/isowind/palettes.dds, the colour-map LUTs used by isowind_pixelart.omwfx.

The palettes are not borrowed from old hardware: they are clustered (k-means) from
Morrowind's own textures, lit at random brightness and with a random warm (torch)
or cool (night) tint, so they hold the greys, browns and greens the game is made of
plus enough darks for shadow. A few accent colours (sky, fire, magic, blood) are
added so those don't collapse into mud.

Each palette is a 64x64x64 cube, stacked along the depth of one 3D texture in the
order of SIZES. Slot 1 in the shader is the first entry (slot 0 is plain posterize).
Cube layout: texel (x, y, z) = (red index, green index, blue index + 64 * slot).
Cells are matched against the palette after gamma-encoding both (x ^ 1/2.2), which
is the space the shader looks colours up in, so darks get their fair share.

Run tools/retro_textures.py first; this samples the textures it wrote.
Requires numpy and Pillow:  python tools/make_palettes.py
"""

import struct
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
TEXTURES = ROOT / "data" / "textures"
OUTPUT = TEXTURES / "isowind" / "palettes.dds"
PREVIEW = ROOT / "palettes-preview.png"

SIZE = 64
GAMMA = 2.2

# Order must match the uPalette choice list in data/shaders/isowind_pixelart.omwfx.
SIZES = [48, 32, 24, 16, 64]

ACCENTS = ["000000", "efe6d2", "6f8fb8", "a9c4e0", "e8913a", "ffd77a", "7a5cc8", "8c1c1c",
           # Light neutrals for haze and overcast sky; without them dither jumps from dark grey to white.
           "8a867e", "aca79c", "cdc8bb"]


def sample_texture_colours(count, rng):
    files = sorted(p for p in TEXTURES.rglob("*.dds") if "isowind" not in p.parts)
    if not files:
        sys.exit(f"no textures in {TEXTURES}; run tools/retro_textures.py first")
    per_file = max(1, count // len(files))
    samples = []
    for path in files:
        rgba = np.asarray(Image.open(path).convert("RGBA"), dtype=np.float64).reshape(-1, 4) / 255.0
        rgba = rgba[rgba[:, 3] > 0.5]
        if len(rgba):
            samples.append(rgba[rng.integers(0, len(rgba), per_file), :3])
    colours = np.concatenate(samples)

    # Fake lighting: random brightness, and a random warm or cool tint.
    # Skewed bright: the gamma-encoded clustering already gives darks plenty of room.
    light = 1.4 * rng.uniform(0.0, 1.0, (len(colours), 1)) ** 0.5
    warm, cool = np.array([1.0, 0.82, 0.6]), np.array([0.68, 0.8, 1.0])
    tint = np.where(rng.random((len(colours), 1)) < 0.5, warm, cool)
    amount = rng.uniform(0.0, 0.4, (len(colours), 1))
    return np.clip(colours * light * (1.0 - amount + amount * tint), 0.0, 1.0)


def kmeans(points, k, rng, iterations=30):
    centres = points[rng.choice(len(points), k, replace=False)]
    for _ in range(iterations):
        nearest = nearest_index(points, centres)
        for i in range(k):
            members = points[nearest == i]
            centres[i] = members.mean(0) if len(members) else points[rng.integers(len(points))]
    return centres


def nearest_index(points, centres):
    out = np.empty(len(points), dtype=np.int64)
    for start in range(0, len(points), 1 << 15):
        chunk = points[start : start + (1 << 15)]
        out[start : start + len(chunk)] = ((chunk[:, None, :] - centres[None, :, :]) ** 2).sum(-1).argmin(1)
    return out


def palette_cube(palette):
    """Nearest palette colour (0-255) for every cell, compared in gamma-encoded space."""
    encoded = (palette / 255.0) ** (1.0 / GAMMA)
    i = (np.arange(SIZE) + 0.5) / SIZE
    b, g, r = np.meshgrid(i, i, i, indexing="ij")
    cells = np.stack([r, g, b], axis=-1).reshape(-1, 3)
    return palette[nearest_index(cells, encoded)].astype(np.uint8).reshape(SIZE, SIZE, SIZE, 3)


def dds_volume_header(width, height, depth):
    DDSD_CAPS, DDSD_HEIGHT, DDSD_WIDTH, DDSD_PITCH = 0x1, 0x2, 0x4, 0x8
    DDSD_PIXELFORMAT, DDSD_DEPTH = 0x1000, 0x800000
    DDPF_RGB = 0x40
    DDSCAPS_COMPLEX, DDSCAPS_TEXTURE = 0x8, 0x1000
    DDSCAPS2_VOLUME = 0x200000

    flags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_DEPTH
    # 24-bit, red in the lowest byte: bytes are stored R, G, B (OSG's "B8G8R8" -> GL_RGB).
    pixel_format = struct.pack("<8I", 32, DDPF_RGB, 0, 24, 0x0000FF, 0x00FF00, 0xFF0000, 0)
    header = struct.pack("<7I", 124, flags, height, width, width * 3, depth, 0)
    header += b"\0" * 44 + pixel_format
    header += struct.pack("<5I", DDSCAPS_COMPLEX | DDSCAPS_TEXTURE, DDSCAPS2_VOLUME, 0, 0, 0)
    assert len(header) == 124
    return b"DDS " + header


def main():
    rng = np.random.default_rng(1)
    colours = sample_texture_colours(150_000, rng)
    encoded = colours ** (1.0 / GAMMA)
    accents = np.array([[int(h[i : i + 2], 16) for i in (0, 2, 4)] for h in ACCENTS], dtype=np.float64)

    cubes, palettes = [], []
    for slot, size in enumerate(SIZES, start=1):
        centres = kmeans(encoded, size - len(accents), rng)
        palette = np.round(np.concatenate([centres ** GAMMA * 255.0, accents]))
        palette = palette[np.argsort(palette @ [0.299, 0.587, 0.114])]
        palettes.append(palette)
        cubes.append(palette_cube(palette))
        print(f"{slot}  Vvardenfell {size}", file=sys.stderr)

    volume = np.concatenate(cubes, axis=0)  # [b + 64 * slot][g][r]
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_bytes(dds_volume_header(SIZE, SIZE, volume.shape[0]) + volume.tobytes())

    swatch = 16
    preview = Image.new("RGB", (swatch * max(SIZES), swatch * len(SIZES)), "black")
    for row, palette in enumerate(palettes):
        for col, c in enumerate(palette.astype(int)):
            preview.paste(tuple(c), (col * swatch, row * swatch, (col + 1) * swatch, (row + 1) * swatch))
    preview.save(PREVIEW)
    print(f"wrote {OUTPUT} ({len(SIZES)} palettes) and {PREVIEW}", file=sys.stderr)


if __name__ == "__main__":
    main()
