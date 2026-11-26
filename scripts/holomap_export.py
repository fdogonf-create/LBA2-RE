#!/usr/bin/env python3
"""Save the holomap's island maps as PNG, to redraw them in high resolution.

Each island map is a 640x480, 256-colour picture inside the game's
holomap.hqr; the colours come from the game palette in ress.hqr. This writes
one PNG per map, named the way the game looks for the redrawn ones:

    holomap_18.png  Citadel island        holomap_36.png  Island CX ...
    (the number is the picture's entry in holomap.hqr; the script prints
    which island each one is)

It also saves the surfaces wrapped around the globes (hologlobe_2.png is
Twinsun). Those stay 256 pixels wide in the game and are redrawn with the
colours the original uses, so a redrawn one can be cleaner but not sharper.

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


def hqr_header(data, index):
    """(size, compressed size, method) of an entry, or None when it has none."""
    (count,) = struct.unpack_from("<I", data, 0)
    if index >= count // 4:
        return None
    (offset,) = struct.unpack_from("<I", data, index * 4)
    if offset <= 0 or offset + 10 > len(data):
        return None
    return struct.unpack_from("<IIh", data, offset) + (offset,)


def hqr_entry(data, index):
    header = hqr_header(data, index)
    if header is None:
        return None
    size, csize, method, offset = header
    body = data[offset + 10:offset + 10 + (size if method == 0 else csize)]
    if method == 0:
        return body
    if method > 2:
        return None
    return expand_lz(body, size, method + 1)


GLOBES = [(2, "Twinsun (the globe)"), (4, "Moon"), (6, "Zeelich"), (8, "Zeelich under the gas"),
          (9, "Sun")]


def write_png(path, pixels, palette, width=WIDTH, height=HEIGHT):
    rows = bytearray()
    for y in range(height):
        rows.append(0)
        for i in pixels[y * width:(y + 1) * width]:
            rows += palette[i * 3:i * 3 + 3]

    def chunk(tag, payload):
        c = struct.pack(">I", len(payload)) + tag + payload
        return c + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
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
    saved = 0
    for n in range(MAP_COUNT):
        index = FIRST_MAP + 2 * n
        header = hqr_header(holo, index)
        try:
            pixels = hqr_entry(holo, index)
        except IndexError:
            pixels = None
        if pixels is None or len(pixels) < WIDTH * HEIGHT:
            print("skipped entry %d (%s): not a 640x480 picture, header %s"
                  % (index, NAMES[n], header))
            continue
        path = os.path.join(out, "holomap_%d.png" % index)
        write_png(path, pixels, palette)
        print("%s  (%s)" % (path, NAMES[n]))
        saved += 1
    if saved < MAP_COUNT:
        # Another release may keep the maps elsewhere: save every 640x480
        # picture in the file, named by its entry, so none is lost.
        (count,) = struct.unpack_from("<I", holo, 0)
        expected = set(FIRST_MAP + 2 * n for n in range(MAP_COUNT))
        for index in range(count // 4):
            if index in expected:
                continue
            header = hqr_header(holo, index)
            if header is None or header[0] != WIDTH * HEIGHT:
                continue
            try:
                pixels = hqr_entry(holo, index)
            except IndexError:
                continue
            if pixels is None:
                continue
            path = os.path.join(out, "holomap_%d.png" % index)
            write_png(path, pixels, palette)
            print("%s  (extra picture found at entry %d)" % (path, index))
            saved += 1
    print("%d maps saved in %s" % (saved, out))

    # The globes' surfaces: 256 texels wide, as tall as the entry allows.
    for index, label in GLOBES:
        try:
            pixels = hqr_entry(holo, index)
        except IndexError:
            pixels = None
        if pixels is None or len(pixels) < 256 * 16:
            print("skipped surface %d (%s)" % (index, label))
            continue
        height = len(pixels) // 256
        path = os.path.join(out, "hologlobe_%d.png" % index)
        write_png(path, pixels, palette, 256, height)
        print("%s  (%s, 256x%d)" % (path, label, height))

if __name__ == "__main__":
    main()
