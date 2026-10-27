#!/usr/bin/env python3
"""Turn a raw read of the reMarkable framebuffer into a PNG (on the PC).

The device is only read: `ssh root@<tablet> cat /dev/fb0 > fb0.raw`.
Format values come from /sys/class/graphics/fb0 on the device:
  bits_per_pixel, stride (bytes per line), visible size (e.g. 1404 x 1872).

usage: fb0-to-png.py fb0.raw out.png WIDTH HEIGHT STRIDE BPP
Supports 8 bit grey, 16 bit RGB565 and 32 bit XRGB (little endian).
"""
import struct
import sys
import zlib


def to_grey(raw, width, height, stride, bpp):
    rows = []
    for y in range(height):
        line = raw[y * stride:y * stride + width * bpp // 8]
        if len(line) < width * bpp // 8:
            raise SystemExit(f"Datei zu kurz für Zeile {y} (stride/Höhe prüfen)")
        if bpp == 8:
            rows.append(bytes(line))
        elif bpp == 16:
            px = struct.unpack(f"<{width}H", line)
            rows.append(bytes(((p >> 11) * 255 // 31 * 299 + ((p >> 5) & 63) * 255 // 63 * 587
                               + (p & 31) * 255 // 31 * 114) // 1000 for p in px))
        elif bpp == 32:
            rows.append(bytes((line[i + 2] * 299 + line[i + 1] * 587 + line[i] * 114) // 1000
                              for i in range(0, width * 4, 4)))
        else:
            raise SystemExit(f"bits_per_pixel {bpp} nicht unterstützt")
    return rows


def write_png(path, rows, width, height):
    def chunk(kind, data):
        body = kind + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)
    data = b"".join(b"\x00" + r for r in rows)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 0, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(data, 9)))
        f.write(chunk(b"IEND", b""))


def main():
    if len(sys.argv) != 7:
        raise SystemExit(__doc__)
    raw_path, out, width, height, stride, bpp = sys.argv[1:]
    width, height, stride, bpp = int(width), int(height), int(stride), int(bpp)
    with open(raw_path, "rb") as f:
        raw = f.read()
    rows = to_grey(raw, width, height, stride, bpp)
    write_png(out, rows, width, height)
    # Share of dark pixels in the right and bottom quarter: > 0 means the
    # UI reaches there (v0.3.1 used only the top left quarter).
    def dark(x0, y0, x1, y1):
        n = sum(1 for y in range(y0, y1) for x in range(x0, x1) if rows[y][x] < 128)
        return 100.0 * n / max(1, (x1 - x0) * (y1 - y0))
    print(f"{out}: {width}x{height}, dunkle Pixel rechtes Viertel {dark(width * 3 // 4, 0, width, height):.2f} %, "
          f"unteres Viertel {dark(0, height * 3 // 4, width, height):.2f} %")


if __name__ == "__main__":
    main()
