"""Build etc/clumzy-icon.ico as square BMP frames Windows can show in the title bar.

The Zub mark is wider than it is tall. Stretching that into a tiny square (or
letting LoadIcon downscale a padded 256px PNG) makes a yellow sliver. This
letterboxes the real glyph into a square and writes uncompressed ICO images
for 16/32, which LoadImage can select correctly.
"""
from __future__ import annotations

import struct
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "etc" / "zub-logo.png"
OUT = ROOT / "etc" / "clumzy-icon.ico"

# Title bar / taskbar first. 256 last for Explorer.
SIZES = (16, 20, 24, 32, 48, 64, 256)


def _load_png_rgba(path: Path) -> tuple[int, int, bytes]:
    import subprocess

    # Prefer Pillow; fall back to .NET so CI/local both work.
    try:
        from PIL import Image

        im = Image.open(path).convert("RGBA")
        return im.size[0], im.size[1], im.tobytes()
    except ImportError:
        pass

    ps = r"""
Add-Type -AssemblyName System.Drawing
$b = [System.Drawing.Bitmap]::FromFile($args[0])
$ms = New-Object System.IO.MemoryStream
$b.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
[Convert]::ToBase64String($ms.ToArray())
$b.Dispose()
"""
    raw_b64 = subprocess.check_output(
        ["powershell", "-NoProfile", "-Command", ps, str(path)],
        text=True,
    ).strip().splitlines()[-1]
    import base64

    png = base64.b64decode(raw_b64)
    return _decode_png(png)


def _decode_png(data: bytes) -> tuple[int, int, bytes]:
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("not a PNG")
    pos = 8
    width = height = None
    chunks: list[bytes] = []
    while pos + 8 <= len(data):
        length = struct.unpack_from(">I", data, pos)[0]
        ctype = data[pos + 4 : pos + 8]
        chunk = data[pos + 8 : pos + 8 + length]
        pos += 12 + length
        if ctype == b"IHDR":
            width, height, bit, color, *_ = struct.unpack(">IIBBBBB", chunk)
            if bit != 8 or color != 6:
                raise ValueError("need 8-bit RGBA PNG")
        elif ctype == b"IDAT":
            chunks.append(chunk)
        elif ctype == b"IEND":
            break
    raw = zlib.decompress(b"".join(chunks))
    stride = width * 4 + 1
    out = bytearray(width * height * 4)
    prev = bytearray(width * 4)
    for y in range(height):
        filt = raw[y * stride]
        scan = bytearray(raw[y * stride + 1 : (y + 1) * stride])
        if filt == 1:
            for i in range(4, len(scan)):
                scan[i] = (scan[i] + scan[i - 4]) & 255
        elif filt == 2:
            for i, v in enumerate(scan):
                scan[i] = (v + prev[i]) & 255
        elif filt == 3:
            for i, v in enumerate(scan):
                a = scan[i - 4] if i >= 4 else 0
                scan[i] = (v + ((a + prev[i]) // 2)) & 255
        elif filt == 4:
            for i, v in enumerate(scan):
                a = scan[i - 4] if i >= 4 else 0
                b = prev[i]
                c = prev[i - 4] if i >= 4 else 0
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pr = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                scan[i] = (v + pr) & 255
        elif filt != 0:
            raise ValueError(f"unsupported PNG filter {filt}")
        prev = scan
        out[y * width * 4 : (y + 1) * width * 4] = scan
    return width, height, bytes(out)


def _bbox(w: int, h: int, rgba: bytes) -> tuple[int, int, int, int]:
    minx, miny, maxx, maxy = w, h, -1, -1
    for y in range(h):
        row = y * w * 4
        for x in range(w):
            i = row + x * 4
            r, g, b, a = rgba[i : i + 4]
            if a > 20 and (r > 30 or g > 30):
                if x < minx:
                    minx = x
                if y < miny:
                    miny = y
                if x > maxx:
                    maxx = x
                if y > maxy:
                    maxy = y
    if maxx < 0:
        return 0, 0, w, h
    return minx, miny, maxx + 1, maxy + 1


def _sample(src: bytes, sw: int, sh: int, x: float, y: float) -> tuple[int, int, int, int]:
    if x < 0 or y < 0 or x >= sw or y >= sh:
        return (0, 0, 0, 0)
    x0, y0 = int(x), int(y)
    x1, y1 = min(x0 + 1, sw - 1), min(y0 + 1, sh - 1)
    fx, fy = x - x0, y - y0

    def pix(px: int, py: int) -> tuple[int, int, int, int]:
        i = (py * sw + px) * 4
        return src[i], src[i + 1], src[i + 2], src[i + 3]

    c00, c10, c01, c11 = pix(x0, y0), pix(x1, y0), pix(x0, y1), pix(x1, y1)

    def mix(p, q, t):
        return tuple(int(a + (b - a) * t) for a, b in zip(p, q))

    return mix(mix(c00, c10, fx), mix(c01, c11, fx), fy)


def _left_glyph_box(w: int, h: int, rgba: bytes) -> tuple[int, int, int, int]:
    """Bounds of the Z only. The full ZC is wide; a square window icon must not include the C."""
    x0, y0, x1, y1 = _bbox(w, h, rgba)
    best_x, best_n = x0 + (x1 - x0) // 2, 10**9
    mid0 = x0 + int((x1 - x0) * 0.35)
    mid1 = x0 + int((x1 - x0) * 0.65)
    for x in range(mid0, mid1):
        n = 0
        for y in range(y0, y1):
            i = (y * w + x) * 4
            r, g, a = rgba[i], rgba[i + 1], rgba[i + 3]
            if a > 20 and (r > 30 or g > 30):
                n += 1
        if n < best_n:
            best_n, best_x = n, x
    zx0, zy0, zx1, zy1 = w, h, -1, -1
    for y in range(y0, y1):
        for x in range(x0, best_x):
            i = (y * w + x) * 4
            r, g, a = rgba[i], rgba[i + 1], rgba[i + 3]
            if a > 20 and (r > 30 or g > 30):
                if x < zx0:
                    zx0 = x
                if y < zy0:
                    zy0 = y
                if x > zx1:
                    zx1 = x
                if y > zy1:
                    zy1 = y
    if zx1 < 0:
        return x0, y0, x0 + (x1 - x0) // 2, y1
    return zx0, zy0, zx1 + 1, zy1 + 1


def _letterbox_square(w: int, h: int, rgba: bytes, size: int) -> bytes:
    x0, y0, x1, y1 = _left_glyph_box(w, h, rgba)
    bw, bh = max(1, x1 - x0), max(1, y1 - y0)
    side = int(max(bw, bh) * 1.06)
    cx = (x0 + x1) / 2.0
    cy = (y0 + y1) / 2.0
    left = cx - side / 2.0
    top = cy - side / 2.0
    out = bytearray(size * size * 4)
    scale = side / float(size)
    for y in range(size):
        sy = top + (y + 0.5) * scale
        for x in range(size):
            sx = left + (x + 0.5) * scale
            r, g, b, a = _sample(rgba, w, h, sx, sy)
            i = (y * size + x) * 4
            out[i : i + 4] = bytes((r, g, b, a))
    return _flatten_on_black(bytes(out), size)


def _flatten_on_black(rgba: bytes, size: int) -> bytes:
    """PNG glow is often RGB-with-zero-alpha. Force a solid black tile so the
    taskbar/title icon fills the square like neighboring apps."""
    out = bytearray(size * size * 4)
    for i in range(size * size):
        r, g, b, a = rgba[i * 4 : i * 4 + 4]
        if a > 32 or r > 24 or g > 20:
            out[i * 4 : i * 4 + 4] = bytes((r, g, b, 255))
        else:
            out[i * 4 : i * 4 + 4] = bytes((0, 0, 0, 255))
    return bytes(out)


def _rgba_to_dib(size: int, rgba: bytes) -> bytes:
    header = struct.pack(
        "<IIIHHIIIIII",
        40,
        size,
        size * 2,
        1,
        32,
        0,
        0,
        0,
        0,
        0,
        0,
    )
    xor = bytearray(size * size * 4)
    for y in range(size):
        src_y = size - 1 - y
        for x in range(size):
            i = (src_y * size + x) * 4
            o = (y * size + x) * 4
            r, g, b, a = rgba[i : i + 4]
            xor[o : o + 4] = bytes((b, g, r, a))
    mask_row = ((size + 31) // 32) * 4
    and_mask = bytes(mask_row * size)
    return header + bytes(xor) + and_mask


def _write_ico(path: Path, images: list[tuple[int, bytes]]) -> None:
    count = len(images)
    offset = 6 + 16 * count
    entries = []
    blobs = []
    for size, rgba in images:
        dib = _rgba_to_dib(size, rgba)
        w = 0 if size >= 256 else size
        entries.append(
            struct.pack("<BBBBHHII", w, w, 0, 0, 1, 32, len(dib), offset)
        )
        blobs.append(dib)
        offset += len(dib)
    path.write_bytes(b"".join([struct.pack("<HHH", 0, 1, count), *entries, *blobs]))


def main() -> int:
    if not SRC.is_file():
        print(f"missing {SRC}", file=sys.stderr)
        return 1
    w, h, rgba = _load_png_rgba(SRC)
    frames = [(s, _letterbox_square(w, h, rgba, s)) for s in SIZES]
    _write_ico(OUT, frames)
    app = next(px for s, px in frames if s == 32)
    app_c = ROOT / "src" / "clumzy_appicon.c"
    lines = [
        '#include "iup.h"',
        "",
        "#define CLUMZY_APPICON_W 32",
        "#define CLUMZY_APPICON_H 32",
        "",
        "static const unsigned char clumzy_appicon_rgba[CLUMZY_APPICON_W * CLUMZY_APPICON_H * 4] = {",
    ]
    for y in range(32):
        row = ", ".join(
            f"0x{app[(y * 32 + x) * 4 + c]:02X}"
            for x in range(32)
            for c in range(4)
        )
        lines.append(f"    {row},")
    lines.extend(
        [
            "};",
            "",
            "Ihandle *createClumzyAppIconImage(void) {",
            "    return IupImageRGBA(CLUMZY_APPICON_W, CLUMZY_APPICON_H, clumzy_appicon_rgba);",
            "}",
            "",
        ]
    )
    app_c.write_text("\n".join(lines), encoding="utf-8")
    print(f"wrote {OUT} ({', '.join(str(s) for s, _ in frames)})")
    print(f"wrote {app_c}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
