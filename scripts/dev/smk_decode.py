#!/usr/bin/env python3
"""Decode a Smacker (.smk) movie to PNG frames and WAV audio tracks.

A pure-Python port of the decoder the engine uses (LIB386/libsmacker,
smacker.c / smk_hufftree.c), so the frames match what the game shows.

Usage:
  smk_decode.py <movie.smk> <out_dir> [--every N] [--max-frames N]

Writes <out_dir>/frame_00000.png ... and track<N>.wav for each audio track,
plus info.txt with the frame rate.
"""
import argparse
import os
import struct
import sys
import zlib

PALMAP = [0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C, 0x20, 0x24, 0x28, 0x2C,
          0x30, 0x34, 0x38, 0x3C, 0x41, 0x45, 0x49, 0x4D, 0x51, 0x55, 0x59, 0x5D,
          0x61, 0x65, 0x69, 0x6D, 0x71, 0x75, 0x79, 0x7D, 0x82, 0x86, 0x8A, 0x8E,
          0x92, 0x96, 0x9A, 0x9E, 0xA2, 0xA6, 0xAA, 0xAE, 0xB2, 0xB6, 0xBA, 0xBE,
          0xC3, 0xC7, 0xCB, 0xCF, 0xD3, 0xD7, 0xDB, 0xDF, 0xE3, 0xE7, 0xEB, 0xEF,
          0xF3, 0xF7, 0xFB, 0xFF]
SIZETABLE = list(range(1, 60)) + [128, 256, 512, 1024, 2048]
BITS = [tuple((b >> i) & 1 for i in range(8)) for b in range(256)]


class Bits:
    """LSB-first bit reader over a flattened bit list."""

    def __init__(self, data):
        self.b = [x for byte in data for x in BITS[byte]]
        self.p = 0

    def bit(self):
        v = self.b[self.p]
        self.p += 1
        return v

    def byte(self):
        p = self.p
        b = self.b
        v = (b[p] | b[p + 1] << 1 | b[p + 2] << 2 | b[p + 3] << 3 | b[p + 4] << 4
             | b[p + 5] << 5 | b[p + 6] << 6 | b[p + 7] << 7)
        self.p = p + 8
        return v


class Tree8:
    """Nodes as parallel lists: left[n] == -1 marks a leaf holding value[n]."""

    def __init__(self, bs):
        self.left, self.right, self.value = [], [], []
        if not bs.bit():
            raise ValueError("huff8: missing start bit")
        self.root = self._build(bs)
        if bs.bit():
            raise ValueError("huff8: bad end bit")

    def _new(self):
        self.left.append(-1)
        self.right.append(-1)
        self.value.append(0)
        return len(self.left) - 1

    def _build(self, bs):
        n = self._new()
        if bs.bit():
            self.left[n] = self._build(bs)
            self.right[n] = self._build(bs)
        else:
            self.value[n] = bs.byte()
        return n

    def lookup(self, bs):
        left, right = self.left, self.right
        b = bs.b
        p = bs.p
        n = self.root
        while left[n] >= 0:
            n = right[n] if b[p] else left[n]
            p += 1
        bs.p = p
        return self.value[n]


class Tree16:
    def __init__(self, bs):
        self.left, self.right, self.value, self.esc = [], [], [], []
        if not bs.bit():
            raise ValueError("huff16: missing start bit")
        lo = Tree8(bs)
        hi = Tree8(bs)
        self.cache_init = []
        for _ in range(3):
            v = bs.byte()
            v |= bs.byte() << 8
            self.cache_init.append(v)
        self.root = self._build(bs, lo, hi)
        if bs.bit():
            raise ValueError("huff16: bad end bit")
        self.cache = [0, 0, 0]

    def _build(self, bs, lo, hi):
        self.left.append(-1)
        self.right.append(-1)
        self.value.append(0)
        self.esc.append(-1)
        n = len(self.left) - 1
        if bs.bit():
            self.left[n] = self._build(bs, lo, hi)
            self.right[n] = self._build(bs, lo, hi)
        else:
            v = lo.lookup(bs) | (hi.lookup(bs) << 8)
            self.value[n] = v
            if v in self.cache_init:
                self.esc[n] = self.cache_init.index(v)
        return n

    def reset(self):
        self.cache = [0, 0, 0]

    def lookup(self, bs):
        left, right = self.left, self.right
        b = bs.b
        p = bs.p
        n = self.root
        while left[n] >= 0:
            n = right[n] if b[p] else left[n]
            p += 1
        bs.p = p
        e = self.esc[n]
        c = self.cache
        v = c[e] if e >= 0 else self.value[n]
        if c[0] != v:
            c[2] = c[1]
            c[1] = c[0]
            c[0] = v
        return v


class Smk:
    def __init__(self, data):
        if data[:3] != b"SMK":
            raise ValueError("not a Smacker file")
        self.version = chr(data[3])
        self.w, self.h, self.nframes, rate, flags = struct.unpack_from("<IIIiI", data, 4)
        self.usf = rate * 1000 if rate > 0 else (-rate * 10 if rate < 0 else 100000)
        if flags & 6:
            raise ValueError("Y-doubled/interlaced Smacker not supported")
        ring = flags & 1
        max_buf = struct.unpack_from("<7I", data, 24)
        tree_size = struct.unpack_from("<I", data, 52)[0]
        rates = struct.unpack_from("<7I", data, 72)
        self.audio = {}
        for t in range(7):
            r = rates[t]
            if r & 0x40000000:
                self.audio[t] = {
                    "compress": 2 if r & 0x0C000000 else (1 if r & 0x80000000 else 0),
                    "bits": 16 if r & 0x20000000 else 8,
                    "ch": 2 if r & 0x10000000 else 1,
                    "rate": r & 0xFFFFFF,
                    "max": max_buf[t],
                }
        n = self.nframes + ring
        pos = 104
        sizes = struct.unpack_from("<%dI" % n, data, pos)
        pos += 4 * n
        self.ftype = data[pos:pos + n]
        pos += n
        bs = Bits(data[pos:pos + tree_size])
        self.trees = [Tree16(bs) for _ in range(4)]  # MMAP, MCLR, FULL, TYPE
        pos += tree_size
        self.chunks = []
        for s in sizes:
            s &= 0xFFFFFFFC
            self.chunks.append(data[pos:pos + s])
            pos += s
        self.frame = bytearray(self.w * self.h)
        self.palette = bytearray(768)

    def _palette(self, p):
        old = bytes(self.palette)
        i = 0
        k = 0
        while i < 256 and k < len(p):
            c = p[k]
            if c & 0x80:
                i += (c & 0x7F) + 1
                k += 1
            elif c & 0x40:
                count = (c & 0x3F) + 1
                src = p[k + 1]
                k += 2
                self.palette[i * 3:(i + count) * 3] = old[src * 3:(src + count) * 3]
                i += count
            else:
                self.palette[i * 3:i * 3 + 3] = bytes(PALMAP[x & 0x3F] for x in p[k:k + 3])
                k += 3
                i += 1

    def _video(self, p):
        w, h = self.w, self.h
        t = self.frame
        bs = Bits(p)
        mmap, mclr, full, ttype = self.trees
        for tr in self.trees:
            tr.reset()
        row = col = 0
        v4 = self.version == "4"
        while row < h:
            unpack = ttype.lookup(bs)
            typ = unpack & 3
            blen = SIZETABLE[(unpack & 0xFC) >> 2]
            tdata = unpack >> 8
            if typ == 1 and v4:
                if bs.bit():
                    typ = 4
                elif bs.bit():
                    typ = 5
            j = 0
            while j < blen and row < h:
                skip = row * w + col
                if typ == 0:
                    u = mclr.lookup(bs)
                    s1 = u >> 8
                    s2 = u & 0xFF
                    m = mmap.lookup(bs)
                    for k in range(4):
                        t[skip] = s1 if m & 1 else s2
                        t[skip + 1] = s1 if m & 2 else s2
                        t[skip + 2] = s1 if m & 4 else s2
                        t[skip + 3] = s1 if m & 8 else s2
                        m >>= 4
                        skip += w
                elif typ == 1:
                    for k in range(4):
                        u = full.lookup(bs)
                        t[skip + 3] = u >> 8
                        t[skip + 2] = u & 0xFF
                        u = full.lookup(bs)
                        t[skip + 1] = u >> 8
                        t[skip] = u & 0xFF
                        skip += w
                elif typ == 3:
                    row4 = bytes((tdata,)) * 4
                    for k in range(4):
                        t[skip:skip + 4] = row4
                        skip += w
                elif typ == 4:
                    for k in range(2):
                        u = full.lookup(bs)
                        a = bytes((u & 0xFF, u & 0xFF, u >> 8, u >> 8))
                        t[skip:skip + 4] = a
                        t[skip + w:skip + w + 4] = a
                        skip += 2 * w
                elif typ == 5:
                    for k in range(2):
                        u = full.lookup(bs)
                        t[skip + 3] = t[skip + w + 3] = u >> 8
                        t[skip + 2] = t[skip + w + 2] = u & 0xFF
                        u = full.lookup(bs)
                        t[skip + 1] = t[skip + w + 1] = u >> 8
                        t[skip] = t[skip + w] = u & 0xFF
                        skip += 2 * w
                # typ 2: void block, keep previous pixels
                col += 4
                if col >= w:
                    col = 0
                    row += 4
                j += 1

    def _audio(self, info, p):
        if info["compress"] == 0:
            return bytes(p)
        if info["compress"] != 1:
            return b""
        out_size = struct.unpack_from("<I", p, 0)[0]
        bs = Bits(p[4:])
        if not bs.bit():
            return b""
        stereo = bs.bit()
        is16 = bs.bit()
        ch = 2 if stereo else 1
        trees = [Tree8(bs)]
        if is16:
            trees.append(Tree8(bs))
        if stereo:
            trees.append(Tree8(bs))
            if is16:
                trees.append(Tree8(bs))
        if is16:
            n = out_size // 2
            out = [0] * n
            if stereo:
                v = bs.byte() << 8
                v |= bs.byte()
                out[1] = v - 0x10000 if v >= 0x8000 else v
            v = bs.byte() << 8
            v |= bs.byte()
            out[0] = v - 0x10000 if v >= 0x8000 else v
            j = ch
            while j < n:
                for c in range(ch):
                    lo_t, hi_t = (trees[0], trees[1]) if c == 0 else (trees[2], trees[3])
                    d = lo_t.lookup(bs) | (hi_t.lookup(bs) << 8)
                    if d >= 0x8000:
                        d -= 0x10000
                    v = out[j - ch] + d
                    out[j] = ((v + 0x8000) & 0xFFFF) - 0x8000
                    j += 1
            return struct.pack("<%dh" % n, *out)
        out = bytearray(out_size)
        if stereo:
            out[1] = bs.byte()
        out[0] = bs.byte()
        j = ch
        while j < out_size:
            for c in range(ch):
                d = trees[c].lookup(bs) if c == 0 else trees[1].lookup(bs)
                out[j] = (out[j - ch] + d) & 0xFF
                j += 1
        return bytes(out)

    def frames(self, want_audio=True):
        """Yield (rgb_bytes, {track: pcm_bytes}) for each frame."""
        for i in range(self.nframes):
            p = self.chunks[i]
            ft = self.ftype[i]
            pos = 0
            if ft & 1:
                size = 4 * p[0]
                self._palette(p[1:size])
                pos = size
            audio = {}
            for track in range(7):
                if ft & (2 << track):
                    size = struct.unpack_from("<I", p, pos)[0]
                    if want_audio and track in self.audio:
                        audio[track] = self._audio(self.audio[track], p[pos + 4:pos + size])
                    pos += size
            self._video(p[pos:])
            yield i, audio


def write_png(path, w, h, frame, palette):
    pal = palette
    raw = bytearray()
    for y in range(h):
        raw.append(0)
        raw += frame[y * w:(y + 1) * w]

    def chunk(tag, body):
        return (struct.pack(">I", len(body)) + tag + body
                + struct.pack(">I", zlib.crc32(tag + body) & 0xFFFFFFFF))
    png = (b"\x89PNG\r\n\x1a\n"
           + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 3, 0, 0, 0))
           + chunk(b"PLTE", bytes(pal))
           + chunk(b"IDAT", zlib.compress(bytes(raw), 6))
           + chunk(b"IEND", b""))
    with open(path, "wb") as f:
        f.write(png)


def write_wav(path, pcm, rate, ch, bits):
    with open(path, "wb") as f:
        f.write(b"RIFF" + struct.pack("<I", 36 + len(pcm)) + b"WAVEfmt "
                + struct.pack("<IHHIIHH", 16, 1, ch, rate, rate * ch * bits // 8, ch * bits // 8, bits)
                + b"data" + struct.pack("<I", len(pcm)) + pcm)


def decode(path, out, every=1, max_frames=None):
    with open(path, "rb") as f:
        s = Smk(f.read())
    os.makedirs(out, exist_ok=True)
    tracks = {t: bytearray() for t in s.audio}
    for i, audio in s.frames():
        for t, pcm in audio.items():
            tracks[t] += pcm
        if i % every == 0:
            write_png(os.path.join(out, "frame_%05d.png" % i), s.w, s.h, s.frame, s.palette)
        if max_frames and i + 1 >= max_frames:
            break
    for t, pcm in tracks.items():
        a = s.audio[t]
        if pcm:
            write_wav(os.path.join(out, "track%d.wav" % t), bytes(pcm), a["rate"], a["ch"], a["bits"])
    fps = 1e6 / s.usf
    with open(os.path.join(out, "info.txt"), "w") as f:
        f.write("frames=%d\nfps=%.4f\nsize=%dx%d\naudio_tracks=%s\n"
                % (s.nframes, fps, s.w, s.h, ",".join(str(t) for t in sorted(tracks) if tracks[t])))
    return s, fps


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("smk")
    ap.add_argument("out")
    ap.add_argument("--every", type=int, default=1)
    ap.add_argument("--max-frames", type=int, default=None)
    a = ap.parse_args()
    s, fps = decode(a.smk, a.out, a.every, a.max_frames)
    print("%s: %d frames, %.3f fps, audio %s" % (a.smk, s.nframes, fps, sorted(s.audio)))


if __name__ == "__main__":
    sys.exit(main())
