"""Draw the Isowind icon: a small isometric pixel-art tile (sand top, two shaded sides) with a gold outline."""
import sys
from PIL import Image, ImageDraw

SIZE = 32


def tile(size):
    s = size / 32
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    top = [(16 * s, 3 * s), (29 * s, 10 * s), (16 * s, 17 * s), (3 * s, 10 * s)]
    left = [(3 * s, 10 * s), (16 * s, 17 * s), (16 * s, 29 * s), (3 * s, 22 * s)]
    right = [(29 * s, 10 * s), (16 * s, 17 * s), (16 * s, 29 * s), (29 * s, 22 * s)]
    d.polygon(left, fill=(120, 84, 48, 255))
    d.polygon(right, fill=(78, 54, 34, 255))
    d.polygon(top, fill=(214, 178, 110, 255))
    for poly in (top, left, right):
        d.line(poly + [poly[0]], fill=(60, 40, 22, 255), width=max(1, int(s)))
    return img


base = tile(SIZE)
sizes = [16, 24, 32, 48, 64, 128, 256]
images = [base.resize((n, n), Image.NEAREST) for n in sizes]
images[-1].save(sys.argv[1], format="ICO", sizes=[(n, n) for n in sizes], append_images=images[:-1])
