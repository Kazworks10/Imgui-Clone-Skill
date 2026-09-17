#!/usr/bin/env python3
"""Extract exact colours from a menu screenshot.

Eyeballing a colour from an image is the number-one source of "looks 70% right"
clones. This reads the actual pixels instead.

    python extract_palette.py shot.png                    # dominant palette
    python extract_palette.py shot.png --at 40,20 --at 500,300
    python extract_palette.py shot.png --grid 8           # coarse colour map
    python extract_palette.py shot.png --region 165,60,480,395   # panel only

Pure stdlib + numpy (numpy optional; a slower fallback is used without it).
PNG is decoded natively. For JPEG/WebP install Pillow.
"""
from __future__ import annotations

import argparse
import struct
import sys
import zlib
from collections import Counter

try:
    import numpy as np
except ImportError:  # pragma: no cover
    np = None


# ── PNG decoding ─────────────────────────────────────────────────────────────
def _paeth(a: int, b: int, c: int) -> int:
    p = a + b - c
    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    return b if pb <= pc else c


def decode_png(path: str):
    """Returns (width, height, rows) with rows as flat RGB bytearrays."""
    with open(path, "rb") as fh:
        data = fh.read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("not a PNG file")

    pos = 8
    idat = bytearray()
    width = height = depth = ctype = 0
    palette = b""
    while pos < len(data):
        (length,) = struct.unpack(">I", data[pos : pos + 4])
        ctag = data[pos + 4 : pos + 8]
        chunk = data[pos + 8 : pos + 8 + length]
        pos += 12 + length

        if ctag == b"IHDR":
            width, height, depth, ctype, _, _, interlace = struct.unpack(">IIBBBBB", chunk)
            if interlace:
                raise ValueError("interlaced PNG unsupported - re-save without interlacing")
            if depth != 8:
                raise ValueError(f"bit depth {depth} unsupported (need 8)")
        elif ctag == b"PLTE":
            palette = chunk
        elif ctag == b"IDAT":
            idat += chunk
        elif ctag == b"IEND":
            break

    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}.get(ctype)
    if channels is None:
        raise ValueError(f"colour type {ctype} unsupported")

    raw = zlib.decompress(bytes(idat))
    stride = width * channels
    out = bytearray(height * stride)
    prev = bytearray(stride)
    src = 0
    for y in range(height):
        ftype = raw[src]
        src += 1
        line = bytearray(raw[src : src + stride])
        src += stride
        if ftype == 1:
            for i in range(channels, stride):
                line[i] = (line[i] + line[i - channels]) & 0xFF
        elif ftype == 2:
            for i in range(stride):
                line[i] = (line[i] + prev[i]) & 0xFF
        elif ftype == 3:
            for i in range(stride):
                left = line[i - channels] if i >= channels else 0
                line[i] = (line[i] + ((left + prev[i]) >> 1)) & 0xFF
        elif ftype == 4:
            for i in range(stride):
                left = line[i - channels] if i >= channels else 0
                ul = prev[i - channels] if i >= channels else 0
                line[i] = (line[i] + _paeth(left, prev[i], ul)) & 0xFF
        elif ftype != 0:
            raise ValueError(f"bad filter type {ftype}")
        out[y * stride : (y + 1) * stride] = line
        prev = line

    # Normalise everything to RGB.
    rgb = bytearray(width * height * 3)
    for i in range(width * height):
        if ctype == 2:
            rgb[i * 3 : i * 3 + 3] = out[i * 3 : i * 3 + 3]
        elif ctype == 6:
            rgb[i * 3 : i * 3 + 3] = out[i * 4 : i * 4 + 3]
        elif ctype == 0:
            g = out[i]
            rgb[i * 3 : i * 3 + 3] = bytes((g, g, g))
        elif ctype == 4:
            g = out[i * 2]
            rgb[i * 3 : i * 3 + 3] = bytes((g, g, g))
        elif ctype == 3:
            p = out[i] * 3
            rgb[i * 3 : i * 3 + 3] = palette[p : p + 3]
    return width, height, rgb


def load_image(path: str):
    if path.lower().endswith(".png"):
        return decode_png(path)
    try:
        from PIL import Image  # type: ignore
    except ImportError:
        raise SystemExit("Non-PNG input needs Pillow:  pip install pillow")
    img = Image.open(path).convert("RGB")
    return img.width, img.height, bytearray(img.tobytes())


# ── analysis ─────────────────────────────────────────────────────────────────
def px(rgb, w, x, y):
    i = (y * w + x) * 3
    return rgb[i], rgb[i + 1], rgb[i + 2]


def hexs(c) -> str:
    return "#%02X%02X%02X" % tuple(c)


def imcolor(c) -> str:
    return "ImColor(%d, %d, %d, 255)" % tuple(c)


def dominant(rgb, w, h, region=None, count=12, quant=8):
    """Quantised histogram: robust, deterministic, no clustering seed."""
    x0, y0, x1, y1 = region or (0, 0, w, h)
    hist: Counter = Counter()
    step = max(1, ((x1 - x0) * (y1 - y0)) // 400_000)
    n = 0
    for y in range(y0, y1):
        for x in range(x0, x1, step):
            r, g, b = px(rgb, w, x, y)
            hist[(r // quant * quant, g // quant * quant, b // quant * quant)] += 1
            n += 1
    return [(c, k / max(1, n)) for c, k in hist.most_common(count)]


def refine(rgb, w, h, approx, region=None, tol=10):
    """Average the true pixels near a quantised bucket to recover the exact value."""
    x0, y0, x1, y1 = region or (0, 0, w, h)
    acc = [0, 0, 0]
    n = 0
    step = max(1, ((x1 - x0) * (y1 - y0)) // 200_000)
    for y in range(y0, y1):
        for x in range(x0, x1, step):
            c = px(rgb, w, x, y)
            if all(abs(c[i] - approx[i]) <= tol for i in range(3)):
                acc[0] += c[0]; acc[1] += c[1]; acc[2] += c[2]
                n += 1
    if not n:
        return approx
    return tuple(round(v / n) for v in acc)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("image")
    ap.add_argument("--at", action="append", default=[], metavar="X,Y",
                    help="sample one exact pixel (repeatable)")
    ap.add_argument("--region", metavar="X0,Y0,X1,Y1",
                    help="restrict the palette to a sub-rectangle")
    ap.add_argument("--grid", type=int, default=0, metavar="N",
                    help="dump an NxN colour grid over the image")
    ap.add_argument("--count", type=int, default=12, help="palette entries")
    args = ap.parse_args()

    w, h, rgb = load_image(args.image)
    print(f"image      : {args.image}")
    print(f"dimensions : {w} x {h}")

    region = None
    if args.region:
        region = tuple(int(v) for v in args.region.split(","))
        print(f"region     : {region}")

    for spec in args.at:
        x, y = (int(v) for v in spec.split(","))
        if not (0 <= x < w and 0 <= y < h):
            print(f"  ({x},{y}) out of bounds")
            continue
        c = px(rgb, w, x, y)
        print(f"  ({x:>4},{y:>4}) {hexs(c):<9} rgb{c}  {imcolor(c)}")

    if args.grid:
        print(f"\ngrid {args.grid}x{args.grid}:")
        for gy in range(args.grid):
            row = []
            for gx in range(args.grid):
                x = min(w - 1, gx * w // args.grid + w // (2 * args.grid))
                y = min(h - 1, gy * h // args.grid + h // (2 * args.grid))
                row.append(hexs(px(rgb, w, x, y)))
            print("  " + " ".join(row))

    print("\ndominant colours (share of pixels):")
    seen: dict[tuple, float] = {}
    for approx, share in dominant(rgb, w, h, region, args.count * 2):
        exact = refine(rgb, w, h, approx, region)
        seen[exact] = seen.get(exact, 0.0) + share   # buckets can refine to one colour
    for exact, share in sorted(seen.items(), key=lambda kv: -kv[1])[: args.count]:
        print(f"  {share*100:5.1f}%  {hexs(exact):<9} {imcolor(exact)}")

    print("\nNOTE: translucent panels over a blurred desktop sample differently "
          "on every background. Sample those over a solid dark backdrop, or take "
          "the value from the opaque part of the menu.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
