#!/usr/bin/env python3
"""Crop and magnify a region of a screenshot for detail comparison.

Reading a menu at 1:1 hides exactly the details that make a clone convincing:
whether a knob is filled or a ring, a 1px decorative line, letter-spacing, the
gradient under a selected tab. Magnify before judging.

    python crop_zoom.py shot.png --crop 380,110,470,140 --zoom 6 --out knob.png
    python crop_zoom.py a.png b.png --crop 175,110,470,150 --zoom 4 --out cmp.png

With two inputs the crops are stacked vertically with a divider, so the same
region of original and clone can be read in one image.
"""
from __future__ import annotations

import argparse
import struct
import sys
import zlib

from extract_palette import load_image


def write_png(path: str, w: int, h: int, rgb: bytearray) -> None:
    raw = bytearray()
    for y in range(h):
        raw.append(0)                                  # filter: none
        raw += rgb[y * w * 3 : (y + 1) * w * 3]

    def chunk(tag: bytes, data: bytes) -> bytes:
        return (struct.pack(">I", len(data)) + tag + data
                + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    with open(path, "wb") as fh:
        fh.write(b"\x89PNG\r\n\x1a\n")
        fh.write(chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)))
        fh.write(chunk(b"IDAT", zlib.compress(bytes(raw), 6)))
        fh.write(chunk(b"IEND", b""))


def crop_scale(w, h, rgb, box, zoom):
    x0, y0, x1, y1 = box
    x0, y0 = max(0, x0), max(0, y0)
    x1, y1 = min(w, x1), min(h, y1)
    cw, ch = (x1 - x0) * zoom, (y1 - y0) * zoom
    out = bytearray(cw * ch * 3)
    for y in range(ch):
        sy = y0 + y // zoom
        for x in range(cw):
            sx = x0 + x // zoom
            s = (sy * w + sx) * 3
            d = (y * cw + x) * 3
            out[d : d + 3] = rgb[s : s + 3]
    return cw, ch, out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("images", nargs="+", help="one image, or two to stack")
    ap.add_argument("--crop", required=True, metavar="X0,Y0,X1,Y1")
    ap.add_argument("--zoom", type=int, default=4)
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    box = tuple(int(v) for v in args.crop.split(","))
    tiles = []
    for path in args.images:
        w, h, rgb = load_image(path)
        tiles.append(crop_scale(w, h, rgb, box, args.zoom))
        print(f"{path}: {w}x{h} -> crop {box} x{args.zoom}")

    if len(tiles) == 1:
        cw, ch, buf = tiles[0]
        write_png(args.out, cw, ch, buf)
    else:
        gap = 8
        cw = max(t[0] for t in tiles)
        ch = sum(t[1] for t in tiles) + gap * (len(tiles) - 1)
        buf = bytearray(b"\xff" * (cw * ch * 3))       # white divider
        y_at = 0
        for tw, th, tb in tiles:
            for y in range(th):
                d = ((y_at + y) * cw) * 3
                buf[d : d + tw * 3] = tb[y * tw * 3 : (y + 1) * tw * 3]
            y_at += th + gap
        write_png(args.out, cw, ch, buf)

    print(f"wrote {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
