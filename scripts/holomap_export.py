#!/usr/bin/env python3
"""Save the holomap's island maps as PNG, to redraw them in high resolution.

Each island map is a 640x480, 256-colour picture inside the game's
holomap.hqr; the colours come from the game palette in ress.hqr. This writes
one PNG per map, named the way the game looks for the redrawn ones:

    holomap_18.png  Citadel island        holomap_36.png  Island CX ...
    (the number is the picture's entry in holomap.hqr; the script prints
    which island each one is)

Put the redrawn pictures, same names, in a folder called `hd` next to the game
data (on the Switch: sdmc:/lba2/hd/). Any 4:3 size works; keep every island
in the same place and shape, because the game draws its arrows and Twinsen on
top at the original positions.

Needs only Python 3, no extra packages:

    python holomap_export.py <game folder> [<output folder>]
"""
import os
import struct
import sys
import zlib

WIDTH, HEIGHT = 640, 480
FIRST_MAP = 18  # HQR_BEGIN_MAP in SOURCES/HOLO.H
MAP_COUNT = 14  # 12 maps, the Citadel after the storm, Celebration after the party
NAMES = [
    "Citadel island", "Sendell's well", "Desert island", "Emerald moon",
    "Otringal", "Celebration island", "Wannies' island (Blafard)",
    "Mosquibees' island", "Knartas", "Island CX", "Elevator",
    "Under Celebration island", "Citadel island after the storm",
    "Celebration island after the party",
]


def find(folder, name):
    for entry in os.listdir(folder):
        if entry.lower() == name:
            return os.path.join(folder, entry)
    sys.exit("No %s in %s" % (name, folder))


def expand_lz(src, size, min_bloc):
    """ExpandLZ (LIB386/SYSTEM/LZ.CPP)."""
    dst = bytearray(size)
    s = d = 0
    while d < size:
        flag = src[s]
        s += 1
        for _ in range(8):
            if d >= size:
                break
            if flag & 1:
                dst[d] = src[s]
                s += 1
                d += 1
            else:
                length = (src[s] & 0x0F) + min_bloc
                offset = (src[s + 1] << 4) | (src[s] >> 4)
                s += 2
                for _ in range(length):
                    if d >= size:
                        break
                    dst[d] = dst[d - offset - 1]
                    d += 1
            flag >>= 1
    return bytes(dst)


def hqr_entry(data, index):
    (offset,) = struct.unpack_from("<I", data, index * 4)
    size, csize, method = struct.unpack_from("<IIh", data, offset)
    body = data[offset + 10:]
    if method == 0:
        return body[:size]
    return expand_lz(body[:csize], size, method + 1)


def write_png(path, pixels, palette):
    rows = bytearray()
    for y in range(HEIGHT):
        rows.append(0)
        for i in pixels[y * WIDTH:(y + 1) * WIDTH]:
            rows += palette[i * 3:i * 3 + 3]

    def chunk(tag, payload):
        c = struct.pack(">I", len(payload)) + tag + payload
        return c + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", WIDTH, HEIGHT, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(bytes(rows), 9))
    png += chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    game = sys.argv[1]
    out = sys.argv[2] if len(sys.argv) > 2 else "holomap_png"
    os.makedirs(out, exist_ok=True)
    with open(find(game, "holomap.hqr"), "rb") as f:
        holo = f.read()
    with open(find(game, "ress.hqr"), "rb") as f:
        palette = hqr_entry(f.read(), 0)[:768]  # RESS_PAL, the game palette
    for n in range(MAP_COUNT):
        index = FIRST_MAP + 2 * n
        pixels = hqr_entry(holo, index)
        if len(pixels) < WIDTH * HEIGHT:
            print("skipped entry %d: not a 640x480 picture" % index)
            continue
        path = os.path.join(out, "holomap_%d.png" % index)
        write_png(path, pixels, palette)
        print("%s  (%s)" % (path, NAMES[n]))


if __name__ == "__main__":
    main()
