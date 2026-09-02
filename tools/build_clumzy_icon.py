"""Build etc/clumzy-icon.ico plus the in-app logo C arrays.

Use the original logo as-is. The only transform is a uniform scale so it fits
the target. Window icons are square (Windows requirement) so the wide mark is
letterboxed. The in-app logo keeps the source aspect ratio.
"""
from __future__ import annotations

import struct
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "etc" / "clumzy-logo.png"
OUT = ROOT / "etc" / "clumzy-icon.ico"
PREVIEW_DIR = ROOT / "dist"

# Title bar / taskbar first. 256 last for Explorer.
SIZES = (16, 20, 24, 32, 48, 64, 256)
LOGO_H = 40
APPICON_SIZE = 32
# ZubCut window chrome — white mark stays visible on light Explorer / title bars.
ICON_FILL = (0x14, 0x14, 0x14, 255)


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


def _scale_to_fit(
    w: int,
    h: int,
    rgba: bytes,
    dw: int,
    dh: int,
    fill: tuple[int, int, int, int] = (0, 0, 0, 0),
) -> bytes:
    """Uniform scale of the original image into dw x dh. No stretch, no crop."""
    try:
        from PIL import Image

        src = Image.frombytes("RGBA", (w, h), rgba)
        canvas = Image.new("RGBA", (dw, dh), fill)
        scale = min(dw / w, dh / h)
        nw = max(1, int(round(w * scale)))
        nh = max(1, int(round(h * scale)))
        resized = src.resize((nw, nh), Image.Resampling.LANCZOS)
        canvas.paste(resized, ((dw - nw) // 2, (dh - nh) // 2), resized)
        return _snap_binary(canvas.tobytes(), dw, dh, fill)
    except ImportError:
        pass

    scale = min(dw / float(w), dh / float(h))
    nw = max(1, int(round(w * scale)))
    nh = max(1, int(round(h * scale)))
    ox = (dw - nw) / 2.0
    oy = (dh - nh) / 2.0
    out = bytearray(dw * dh * 4)
    for y in range(dh):
        for x in range(dw):
            i = (y * dw + x) * 4
            out[i : i + 4] = bytes(fill)
            sx = (x - ox + 0.5) * (w / float(nw)) - 0.5
            sy = (y - oy + 0.5) * (h / float(nh)) - 0.5
            r, g, b, a = _sample(rgba, w, h, sx, sy)
            if a == 0:
                continue
            ia = a / 255.0
            out[i] = int(r * ia + fill[0] * (1.0 - ia))
            out[i + 1] = int(g * ia + fill[1] * (1.0 - ia))
            out[i + 2] = int(b * ia + fill[2] * (1.0 - ia))
            out[i + 3] = min(255, fill[3] + a)
    return _snap_binary(bytes(out), dw, dh, fill)


def _snap_binary(
    rgba: bytes, dw: int, dh: int, fill: tuple[int, int, int, int]
) -> bytes:
    """Keep the original 1-bit white mark. Scale only; no grey smear."""
    out = bytearray(rgba)
    fr, fg, fb, fa = fill
    for i in range(0, dw * dh * 4, 4):
        r, g, b, a = out[i : i + 4]
        if a >= 128 and (r + g + b) >= 384:
            out[i : i + 4] = bytes((255, 255, 255, 255))
        else:
            out[i : i + 4] = bytes((fr, fg, fb, fa))
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
    width: int,
    height: int,
    rgba: bytes,
) -> None:
    lines = [
        '#include "iup.h"',
        "",
        f"#define {macro_w} {width}",
        f"#define {macro_h} {height}",
        "",
        f"static const unsigned char {array_name}[{macro_w} * {macro_h} * 4] = {{",
    ]
    for y in range(height):
        row = ", ".join(
            f"0x{rgba[(y * width + x) * 4 + c]:02X}"
            for x in range(width)
            for c in range(4)
        )
        lines.append(f"    {row},")
    lines.extend(
        [
            "};",
            "",
            f"Ihandle *{fn_name}(void) {{",
            f"    Ihandle *img = IupImageRGBA({macro_w}, {macro_h}, {array_name});",
            '    IupSetAttribute(img, "AUTOSCALE", "NO");',
            "    return img;",
            "}",
            "",
        ]
    )
    path.write_text("\n".join(lines), encoding="utf-8")


def _composite(rgba: bytes, width: int, height: int, bg: tuple[int, int, int]) -> bytes:
    out = bytearray(width * height * 4)
    for y in range(height):
        for x in range(width):
            i = (y * width + x) * 4
            r, g, b, a = rgba[i : i + 4]
            ia = a / 255.0
            out[i] = int(r * ia + bg[0] * (1.0 - ia))
            out[i + 1] = int(g * ia + bg[1] * (1.0 - ia))
            out[i + 2] = int(b * ia + bg[2] * (1.0 - ia))
            out[i + 3] = 255
    return bytes(out)


def _write_preview_png(path: Path, width: int, height: int, rgba: bytes) -> None:
    try:
        from PIL import Image

        Image.frombytes("RGBA", (width, height), rgba).save(path)
        return
    except ImportError:
        pass
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        raw.extend(rgba[y * width * 4 : (y + 1) * width * 4])
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)

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
    frames = [(s, _scale_to_fit(w, h, rgba, s, s, ICON_FILL)) for s in SIZES]
    _write_ico(OUT, frames)
    app = next(px for s, px in frames if s == APPICON_SIZE)
    _write_c_rgba(
        ROOT / "src" / "clumzy_appicon.c",
        "CLUMZY_APPICON_W",
        "CLUMZY_APPICON_H",
        "clumzy_appicon_rgba",
        "createClumzyAppIconImage",
        APPICON_SIZE,
        APPICON_SIZE,
        app,
    )
    logo_w = max(1, int(round(LOGO_H * w / float(h))))
    logo = _scale_to_fit(w, h, rgba, logo_w, LOGO_H)
    _write_c_rgba(
        ROOT / "src" / "clumzy_logo.c",
        "CLUMZY_LOGO_W",
        "CLUMZY_LOGO_H",
        "clumzy_logo_rgba",
        "createClumzyLogoImage",
        logo_w,
        LOGO_H,
        logo,
    )
    PREVIEW_DIR.mkdir(exist_ok=True)
    for s, px in frames:
        _write_preview_png(PREVIEW_DIR / f"icon-preview-{s}.png", s, s, px)
    _write_preview_png(PREVIEW_DIR / f"logo-preview-{logo_w}x{LOGO_H}.png", logo_w, LOGO_H, logo)
    _write_preview_png(
        PREVIEW_DIR / f"logo-preview-{logo_w}x{LOGO_H}-dark.png",
        logo_w,
        LOGO_H,
        _composite(logo, logo_w, LOGO_H, (0x14, 0x14, 0x14)),
    )
    print(f"wrote {OUT} ({', '.join(str(s) for s, _ in frames)})")
    print(f"wrote src/clumzy_appicon.c")
    print(f"wrote src/clumzy_logo.c ({logo_w}x{LOGO_H})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
