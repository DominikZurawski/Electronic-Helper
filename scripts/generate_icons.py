#!/usr/bin/env python3
import struct
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ASSETS = ROOT / "assets"
ASSETS.mkdir(parents=True, exist_ok=True)

PNG_PATH = ASSETS / "icon.png"
ICO_PATH = ASSETS / "icon.ico"

SIZES = [16, 32, 48, 64, 128, 256]
BG = (11, 31, 42, 255)  # dark teal


def make_png(path: Path, size: int) -> bytes:
    # Solid RGBA image
    row = bytes(BG) * size
    raw = b"".join(b"\x00" + row for _ in range(size))
    compressed = zlib.compress(raw, level=9)

    def chunk(tag: bytes, data: bytes) -> bytes:
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    ihdr = struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", compressed) + chunk(b"IEND", b"")
    path.write_bytes(png)
    return png


def make_ico(path: Path, images: list[tuple[int, bytes]]) -> None:
    # ICO header: reserved(0), type(1), count(n)
    header = struct.pack("<HHH", 0, 1, len(images))
    entries = []
    data = b""
    offset = 6 + (16 * len(images))

    for size, png_bytes in images:
      width = 0 if size == 256 else size
      height = 0 if size == 256 else size
      color_count = 0
      reserved = 0
      planes = 1
      bit_count = 32
      bytes_in_res = len(png_bytes)
      entries.append(struct.pack("<BBBBHHII", width, height, color_count, reserved, planes, bit_count, bytes_in_res, offset))
      data += png_bytes
      offset += bytes_in_res

    path.write_bytes(header + b"".join(entries) + data)


def main() -> None:
    images = []
    for size in SIZES:
        png_path = ASSETS / f"icon_{size}.png"
        png_bytes = make_png(png_path, size)
        images.append((size, png_bytes))
        if size == 256:
            PNG_PATH.write_bytes(png_bytes)
    make_ico(ICO_PATH, images)
    print(f"Generated: {PNG_PATH}")
    print(f"Generated: {ICO_PATH}")
    for size in SIZES:
        print(f"Generated: {ASSETS / f'icon_{size}.png'}")


if __name__ == "__main__":
    main()
