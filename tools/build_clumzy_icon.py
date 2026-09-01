"""Build etc/clumzy-icon.ico plus the in-app logo C arrays.

The Zub ZC mark is wider than it is tall and sits in a square PNG with a lot of
empty margin. Crop the full mark (both letters), letterbox it into a square with
a transparent surround, and write uncompressed ICO frames Windows can pick at
title-bar / taskbar sizes.
"""
from __future__ import annotations

import struct
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "etc" / "zub-logo.png"
OUT = ROOT / "etc" / "clumzy-icon.ico"
PREVIEW_DIR = ROOT / "dist"

# Title bar / taskbar first. 256 last for Explorer.
SIZES = (16, 20, 24, 32, 48, 64, 256)
# Breathing room around the glow so the square does not clip the ZC.
PAD_FRAC = 0.08


def _load_png_rgba(path: Path) -> tuple[int, int, bytes]:
    import subprocess

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
    """Bounds of the full ZC, including yellow glow that may have alpha 0."""
    minx, miny, maxx, maxy = w, h, -1, -1
    for y in range(h):
        row = y * w * 4
        for x in range(w):
            i = row + x * 4
            r, g, a = rgba[i], rgba[i + 1], rgba[i + 3]
            if a > 16 or r > 24 or g > 20:
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


def _letterbox_square(w: int, h: int, rgba: bytes, size: int) -> bytes:
    x0, y0, x1, y1 = _bbox(w, h, rgba)
    bw, bh = max(1, x1 - x0), max(1, y1 - y0)
    side = max(bw, bh) * (1.0 + 2.0 * PAD_FRAC)
    cx = (x0 + x1) / 2.0
    cy = (y0 + y1) / 2.0
    left = cx - side / 2.0
    top = cy - side / 2.0
    try:
        from PIL import Image

        src = Image.frombytes("RGBA", (w, h), rgba)
        crop = (
            int(round(left)),
            int(round(top)),
            int(round(left + side)),
            int(round(top + side)),
        )
        master = Image.new("RGBA", (crop[2] - crop[0], crop[3] - crop[1]), (0, 0, 0, 0))
        src_crop = src.crop(crop)
        master.paste(src_crop, (0, 0))
        resized = master.resize((size, size), Image.Resampling.LANCZOS)
        out = bytearray(resized.tobytes())
    except ImportError:
        out = bytearray(size * size * 4)
        scale = side / float(size)
        for y in range(size):
            sy = top + (y + 0.5) * scale
            for x in range(size):
                sx = left + (x + 0.5) * scale
                r, g, b, a = _sample(rgba, w, h, sx, sy)
                i = (y * size + x) * 4
                out[i : i + 4] = bytes((r, g, b, a))
    return _restore_glow_alpha(bytes(out), size)


def _restore_glow_alpha(rgba: bytes, size: int) -> bytes:
    """Keep the ZC glow, punch the surround to real transparency.

    Source glow is often RGB with alpha 0. Empty / near-black padding must stay
    0,0,0,0 so the title bar and taskbar do not paint a black square.
    """
    out = bytearray(rgba)
    for i in range(size * size):
        r, g, b, a = out[i * 4 : i * 4 + 4]
        if a < 8 and (r > 16 or g > 12):
            a = max(r, g, b)
        if a < 16 and r < 20 and g < 16 and b < 16:
            out[i * 4 : i * 4 + 4] = bytes((0, 0, 0, 0))
        else:
            out[i * 4 + 3] = a
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
    and_mask = bytearray(mask_row * size)
    for y in range(size):
        src_y = size - 1 - y
        for x in range(size):
            a = rgba[(src_y * size + x) * 4 + 3]
            if a < 16:
                and_mask[y * mask_row + (x >> 3)] |= 0x80 >> (x & 7)
    return header + bytes(xor) + bytes(and_mask)


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


def _write_c_rgba(
    path: Path,
    macro_w: str,
    macro_h: str,
    array_name: str,
    fn_name: str,
    size: int,
    rgba: bytes,
) -> None:
    lines = [
        '#include "iup.h"',
        "",
        f"#define {macro_w} {size}",
        f"#define {macro_h} {size}",
        "",
        f"static const unsigned char {array_name}[{macro_w} * {macro_h} * 4] = {{",
    ]
    for y in range(size):
        row = ", ".join(
            f"0x{rgba[(y * size + x) * 4 + c]:02X}"
            for x in range(size)
            for c in range(4)
        )
        lines.append(f"    {row},")
    lines.extend(
        [
            "};",
            "",
            f"Ihandle *{fn_name}(void) {{",
            f"    return IupImageRGBA({macro_w}, {macro_h}, {array_name});",
            "}",
            "",
        ]
    )
    path.write_text("\n".join(lines), encoding="utf-8")


def _on_checker(rgba: bytes, size: int) -> bytes:
    out = bytearray(size * size * 4)
    for y in range(size):
        for x in range(size):
            i = (y * size + x) * 4
            r, g, b, a = rgba[i : i + 4]
            cell = 200 if ((x // 4) + (y // 4)) % 2 == 0 else 120
            ia = a / 255.0
            out[i] = int(r * ia + cell * (1.0 - ia))
            out[i + 1] = int(g * ia + cell * (1.0 - ia))
            out[i + 2] = int(b * ia + cell * (1.0 - ia))
            out[i + 3] = 255
    return bytes(out)


def _write_preview_png(path: Path, size: int, rgba: bytes) -> None:
    try:
        from PIL import Image

        Image.frombytes("RGBA", (size, size), rgba).save(path)
        return
    except ImportError:
        pass
    raw = bytearray()
    for y in range(size):
        raw.append(0)
        raw.extend(rgba[y * size * 4 : (y + 1) * size * 4])
    ihdr = struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0)

    def chunk(tag: bytes, data: bytes) -> bytes:
        return (
            struct.pack(">I", len(data))
            + tag
            + data
            + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
        )

    path.write_bytes(
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", ihdr)
        + chunk(b"IDAT", zlib.compress(bytes(raw), 9))
        + chunk(b"IEND", b"")
    )


def main() -> int:
    if not SRC.is_file():
        print(f"missing {SRC}", file=sys.stderr)
        return 1
    w, h, rgba = _load_png_rgba(SRC)
    frames = [(s, _letterbox_square(w, h, rgba, s)) for s in SIZES]
    _write_ico(OUT, frames)
    app = next(px for s, px in frames if s == 32)
    _write_c_rgba(
        ROOT / "src" / "clumzy_appicon.c",
        "CLUMZY_APPICON_W",
        "CLUMZY_APPICON_H",
        "clumzy_appicon_rgba",
        "createClumzyAppIconImage",
        32,
        app,
    )
    logo = _letterbox_square(w, h, rgba, 48)
    _write_c_rgba(
        ROOT / "src" / "clumzy_logo.c",
        "CLUMZY_LOGO_W",
        "CLUMZY_LOGO_H",
        "clumzy_logo_rgba",
        "createClumzyLogoImage",
        48,
        logo,
    )
    PREVIEW_DIR.mkdir(exist_ok=True)
    for s, px in frames:
        _write_preview_png(PREVIEW_DIR / f"icon-preview-{s}.png", s, px)
        _write_preview_png(
            PREVIEW_DIR / f"icon-preview-{s}-check.png", s, _on_checker(px, s)
        )
    _write_preview_png(PREVIEW_DIR / "logo-preview-48.png", 48, logo)
    _write_preview_png(
        PREVIEW_DIR / "logo-preview-48-check.png", 48, _on_checker(logo, 48)
    )
    print(f"wrote {OUT} ({', '.join(str(s) for s, _ in frames)})")
    print("wrote src/clumzy_appicon.c")
    print("wrote src/clumzy_logo.c")
    return 0


if __name__ == "__main__":
    sys.exit(main())
