#!/usr/bin/env python3
"""Dubbing toolkit for LBA2: export voices, videos and the dialogue script,
and pack recorded lines back into .VOX banks.

Only reads the game data; every file it writes goes under --out.

  export   voices (.VOX -> 16-bit PCM WAV), videos (VIDEO.HQR -> .smk, and
           .mp4 when ffmpeg is available) and the script (CSV + per-bank TXT
           pairing each WAV with its text id and its text in two languages).
  repack   build new <LANG>_<bank>.VOX files from recorded WAVs, keeping the
           original line for every slot that has no recording.

VOX layout (SOURCES/MESSAGE.CPP InitFileNar / Speak / PlaySpeakVoc):
  - An HQR whose slot n is the voice of slot n of the matching TEXT.HQR order
    bank (FindText returns the slot, not the text id).
  - Each entry is a RIFF WAV (IMA ADPCM, mono, 22050 Hz, 512-byte blocks) whose
    first byte, normally 'R', is a continuation flag: non-zero means another
    entry header + WAV follows directly after this one and plays next. Long
    lines are split this way because each part must fit in BufSpeak.

Usage:
  lba2_dub.py export --game <Common dir> --out <dir> [--voice-lang EN]
                     [--text-lang ES] [--ffmpeg PATH] [--skip-voices] [--skip-videos]
  lba2_dub.py repack --game <Common dir> --recordings <dir> --out <dir>
                     [--base-lang EN] [--prefix EN] [--no-match-volume]
"""
import argparse
import csv
import math
import os
import shutil
import struct
import subprocess
import sys
import wave

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from hqr_inspect import entries, decompress_entry  # noqa: E402
from text_dump import bank_rows, BANKS  # noqa: E402

TEXT_LANGS = {"EN": 0, "FR": 1, "DE": 2, "ES": 3, "IT": 4, "PT": 5}
VOICE_BANKS = ["gam"] + ["%03d" % i for i in range(12)]
RESS_ACFLIST = 48  # SOURCES/COMMON.H
GAME_RATE = 22050
BLOCK_ALIGN = 512
SAMPLES_PER_BLOCK = (BLOCK_ALIGN - 4) * 2 + 1  # 1017
# Largest single part in the retail banks is 249136 bytes; staying under it
# keeps every part inside BufSpeak for any engine build.
MAX_PART_BYTES = 240 * 1024

# --- IMA ADPCM -----------------------------------------------------------

STEP = [7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37,
        41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173,
        190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658,
        724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
        2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
        7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818,
        18500, 20350, 22385, 24623, 27086, 29794, 32767]
INDEX_ADJ = [-1, -1, -1, -1, 2, 4, 6, 8]

# DIFF[index][nibble] and NEXT[index][nibble]: precomputed decoder steps.
DIFF = []
NEXT = []
for _i in range(89):
    _s = STEP[_i]
    _d, _n = [], []
    for _nib in range(16):
        diff = _s >> 3
        if _nib & 4:
            diff += _s
        if _nib & 2:
            diff += _s >> 1
        if _nib & 1:
            diff += _s >> 2
        _d.append(-diff if _nib & 8 else diff)
        _n.append(min(88, max(0, _i + INDEX_ADJ[_nib & 7])))
    DIFF.append(_d)
    NEXT.append(_n)


def ima_decode(data, block_align):
    out = []
    append = out.append
    pos = 0
    n = len(data)
    while pos + 4 <= n:
        block = data[pos:pos + block_align]
        pos += block_align
        pred = struct.unpack_from("<h", block, 0)[0]
        idx = min(88, block[2])
        append(pred)
        for byte in block[4:]:
            for nib in (byte & 0x0F, byte >> 4):
                pred += DIFF[idx][nib]
                if pred > 32767:
                    pred = 32767
                elif pred < -32768:
                    pred = -32768
                idx = NEXT[idx][nib]
                append(pred)
    return out


def ima_encode(samples):
    """Mono 16-bit samples -> IMA ADPCM blocks (BLOCK_ALIGN bytes each)."""
    out = bytearray()
    idx = 0
    for start in range(0, len(samples), SAMPLES_PER_BLOCK):
        chunk = samples[start:start + SAMPLES_PER_BLOCK]
        if len(chunk) < SAMPLES_PER_BLOCK:
            chunk = chunk + [0] * (SAMPLES_PER_BLOCK - len(chunk))
        pred = chunk[0]
        out += struct.pack("<hBB", pred, idx, 0)
        nibs = []
        for s in chunk[1:]:
            step = STEP[idx]
            diff = s - pred
            nib = 0
            if diff < 0:
                nib = 8
                diff = -diff
            if diff >= step:
                nib |= 4
                diff -= step
            if diff >= step >> 1:
                nib |= 2
                diff -= step >> 1
            if diff >= step >> 2:
                nib |= 1
            pred += DIFF[idx][nib]
            pred = max(-32768, min(32767, pred))
            idx = NEXT[idx][nib]
            nibs.append(nib)
        for i in range(0, len(nibs), 2):
            out.append(nibs[i] | (nibs[i + 1] << 4))
    return bytes(out)


# --- WAV -----------------------------------------------------------------

def riff_chunks(blob):
    if blob[8:12] != b"WAVE":
        raise ValueError("not a WAVE file")
    pos = 12
    chunks = {}
    while pos + 8 <= len(blob):
        cid = blob[pos:pos + 4]
        (size,) = struct.unpack_from("<I", blob, pos + 4)
        chunks.setdefault(cid, blob[pos + 8:pos + 8 + size])
        pos += 8 + size + (size & 1)
    return chunks


def read_any_wav(path):
    """Return (mono 16-bit samples, rate) for PCM 8/16/24/32, float32 or IMA ADPCM."""
    with open(path, "rb") as f:
        blob = f.read()
    ch = riff_chunks(blob)
    fmt, data = ch[b"fmt "], ch[b"data"]
    tag, nch, rate, _, align, bits = struct.unpack_from("<HHIIHH", fmt, 0)
    if tag == 0xFFFE:  # WAVE_FORMAT_EXTENSIBLE: the real tag is the GUID head
        tag = struct.unpack_from("<H", fmt, 24)[0]
    if tag == 17:
        if nch != 1:
            raise ValueError("%s: stereo IMA ADPCM is not supported" % path)
        return ima_decode(data, align), rate
    width = bits // 8
    frames = len(data) // (width * nch)
    if tag == 3 and bits == 32:
        vals = struct.unpack("<%df" % (frames * nch), data[:frames * nch * 4])
        vals = [int(max(-1.0, min(1.0, v)) * 32767) for v in vals]
    elif tag == 1 and bits == 16:
        vals = list(struct.unpack("<%dh" % (frames * nch), data[:frames * nch * 2]))
    elif tag == 1 and bits == 8:
        vals = [(b - 128) << 8 for b in data[:frames * nch]]
    elif tag == 1 and bits == 24:
        vals = [int.from_bytes(data[i:i + 3], "little", signed=True) >> 8
                for i in range(0, frames * nch * 3, 3)]
    elif tag == 1 and bits == 32:
        vals = [v >> 16 for v in struct.unpack("<%di" % (frames * nch), data[:frames * nch * 4])]
    else:
        raise ValueError("%s: unsupported WAV format (tag %d, %d bits)" % (path, tag, bits))
    if nch > 1:
        vals = [sum(vals[i:i + nch]) // nch for i in range(0, len(vals), nch)]
    return vals, rate


def write_pcm_wav(path, samples, rate):
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(struct.pack("<%dh" % len(samples), *samples))


def adpcm_wav_bytes(samples):
    """A game-style RIFF: IMA ADPCM mono 22050 Hz, 512-byte blocks."""
    data = ima_encode(samples)
    byte_rate = GAME_RATE * BLOCK_ALIGN // SAMPLES_PER_BLOCK
    fmt = struct.pack("<HHIIHHHH", 17, 1, GAME_RATE, byte_rate, BLOCK_ALIGN, 4,
                      2, SAMPLES_PER_BLOCK)
    fact = struct.pack("<I", len(samples))
    body = (b"WAVE" + b"fmt " + struct.pack("<I", len(fmt)) + fmt
            + b"fact" + struct.pack("<I", 4) + fact
            + b"data" + struct.pack("<I", len(data)) + data)
    return b"RIFF" + struct.pack("<I", len(body)) + body


def resample(samples, src, dst):
    """Windowed-sinc resampler (pure Python; fine for single lines)."""
    if src == dst or not samples:
        return list(samples)
    ratio = dst / src
    cutoff = min(1.0, ratio) * 0.95
    half = 16 if ratio >= 1 else int(math.ceil(16 / ratio))
    n_out = int(len(samples) * ratio)
    out = []
    last = len(samples) - 1
    for j in range(n_out):
        t = j / ratio
        c = int(t)
        acc = 0.0
        wsum = 0.0
        for k in range(c - half + 1, c + half + 1):
            if k < 0 or k > last:
                continue
            x = (t - k) * cutoff
            sinc = 1.0 if x == 0 else math.sin(math.pi * x) / (math.pi * x)
            win = 0.5 + 0.5 * math.cos(math.pi * (t - k) / (half + 1))
            w = sinc * win
            acc += samples[k] * w
            wsum += w
        out.append(int(max(-32768, min(32767, acc / wsum if wsum else 0))))
    return out


def rms(samples):
    if not samples:
        return 0.0
    return math.sqrt(sum(s * s for s in samples) / len(samples))


# --- VOX -----------------------------------------------------------------

def vox_parts(data, off):
    """All WAV parts of one voice entry, following the continuation flag."""
    parts = []
    while off + 10 <= len(data):
        size, csize, method = struct.unpack_from("<IIh", data, off)
        wav = bytearray(decompress_entry(data, off, size, csize, method))
        flag = wav[0]
        wav[0] = ord("R")
        parts.append(bytes(wav))
        if not flag:
            break
        off += 10 + csize
    return parts


def vox_line_samples(data, off):
    samples = []
    for wav in vox_parts(data, off):
        ch = riff_chunks(wav)
        tag, _, _, _, align, _ = struct.unpack_from("<HHIIHH", ch[b"fmt "], 0)
        if tag == 17:
            part = ima_decode(ch[b"data"], align)
            if b"fact" in ch:  # drop the padding of the last ADPCM block
                part = part[:struct.unpack_from("<I", ch[b"fact"])[0]]
        else:
            part = list(struct.unpack("<%dh" % (len(ch[b"data"]) // 2), ch[b"data"]))
        samples += part
    return samples


def split_points(samples, max_samples):
    """Cut long lines at the quietest 20 ms window in the last 2 s of each part."""
    cuts = []
    start = 0
    win = GAME_RATE // 50
    while len(samples) - start > max_samples:
        hi = start + max_samples
        lo = max(start + max_samples // 2, hi - 2 * GAME_RATE)
        best, best_e = hi, None
        for p in range(lo, hi - win, win):
            e = sum(abs(s) for s in samples[p:p + win])
            if best_e is None or e < best_e:
                best, best_e = p + win // 2, e
        cuts.append(best)
        start = best
    return cuts


def build_entry(samples):
    """Header(s) + WAV part(s) for one voice line, flags set for chaining."""
    max_samples = (MAX_PART_BYTES - 64) // BLOCK_ALIGN * SAMPLES_PER_BLOCK
    bounds = [0] + split_points(samples, max_samples) + [len(samples)]
    out = bytearray()
    for i in range(len(bounds) - 1):
        wav = bytearray(adpcm_wav_bytes(samples[bounds[i]:bounds[i + 1]]))
        wav[0] = 1 if i < len(bounds) - 2 else 0
        out += struct.pack("<IIh", len(wav), len(wav), 0) + wav
    return bytes(out)


def raw_entry_span(data, off):
    """Bytes of an original entry including any chained parts."""
    start = off
    while True:
        size, csize, method = struct.unpack_from("<IIh", data, off)
        flag = decompress_entry(data, off, size, csize, method)[0]
        off += 10 + csize
        if not flag:
            return data[start:off]


# --- export --------------------------------------------------------------

def find_ffmpeg(explicit):
    if explicit:
        return explicit
    return shutil.which("ffmpeg")


def export_voices(common, out, voice_lang, text_lang, ref_lang):
    vox_dir = os.path.join(common, "VOX")
    text_hqr = os.path.join(common, "TEXT.HQR")
    voice_root = os.path.join(out, "1_Voces_originales", voice_lang)
    script_root = os.path.join(out, "3_Guion")
    os.makedirs(script_root, exist_ok=True)
    rows = []
    for bank in VOICE_BANKS:
        path = os.path.join(vox_dir, "%s_%s.VOX" % (voice_lang, bank.upper()))
        if not os.path.exists(path):
            print("  (no hay %s)" % path)
            continue
        ents, data = entries(path)
        bi = BANKS.index(bank)
        tgt = bank_rows(text_hqr, TEXT_LANGS[text_lang], bi)
        ref = bank_rows(text_hqr, TEXT_LANGS[ref_lang], bi)
        os.makedirs(os.path.join(voice_root, bank), exist_ok=True)
        lines_txt = []
        count = 0
        for slot, off, size, _, _ in ents[:-1]:
            if not size:
                continue
            samples = vox_line_samples(data, off)
            name = "%s_%04d.wav" % (bank, slot)
            write_pcm_wav(os.path.join(voice_root, bank, name), samples, GAME_RATE)
            text_id = tgt[slot][0] if slot < len(tgt) else ""
            t_txt = tgt[slot][2] if slot < len(tgt) else ""
            r_txt = ref[slot][2] if slot < len(ref) else ""
            dur = len(samples) / GAME_RATE
            parts = len(vox_parts(data, off))
            rows.append([bank + "/" + name, bank, slot, text_id, "%.2f" % dur,
                         t_txt.replace("@", " ").strip(), r_txt.replace("@", " ").strip(),
                         parts, ""])
            lines_txt.append("[%s]  id %s  (%.1f s)\n%s: %s\n%s: %s\n" % (
                name, text_id, dur, text_lang, t_txt.replace("@", "\n    "),
                ref_lang, r_txt.replace("@", "\n    ")))
            count += 1
        with open(os.path.join(script_root, "guion_%s.txt" % bank), "w", encoding="utf-8") as f:
            f.write("\n".join(lines_txt))
        print("  %s_%s.VOX: %d frases" % (voice_lang, bank.upper(), count))
    header = ["archivo", "banco", "slot", "id_texto", "duracion_seg",
              "texto_" + text_lang.lower(), "texto_" + ref_lang.lower(), "partes", "estado"]
    csv_path = os.path.join(script_root, "guion_doblaje.csv")
    # utf-8-sig + ';' so a Spanish-locale Excel opens it in columns.
    with open(csv_path, "w", encoding="utf-8-sig", newline="") as f:
        w = csv.writer(f, delimiter=";")
        w.writerow(header)
        w.writerows(rows)
    total = sum(float(r[4]) for r in rows)
    print("  %d frases, %.1f minutos de audio -> %s" % (len(rows), total / 60, csv_path))


def export_videos(common, out, ffmpeg):
    """Extract each movie as .smk, decode it to PNG frames + WAV with the same
    decoder the engine uses, and build an .mp4 from those when ffmpeg exists.
    Encoding from the decoded frames (not from the .smk) keeps players that
    drop or mis-decode Smacker delta frames out of the picture."""
    from smk_decode import decode
    hqr = os.path.join(common, "VIDEO", "VIDEO.HQR")
    ress = os.path.join(common, "RESS.HQR")
    ents, data = entries(ress)
    _, off, size, csize, method = ents[RESS_ACFLIST]
    names = [n.decode("ascii") for n in decompress_entry(data, off, size, csize, method).split()]
    smk_dir = os.path.join(out, "2_Videos", "smk")
    png_root = os.path.join(out, "2_Videos", "fotogramas")
    mp4_dir = os.path.join(out, "2_Videos", "mp4")
    os.makedirs(smk_dir, exist_ok=True)
    ents, data = entries(hqr)
    for i, name in enumerate(names):
        if i >= len(ents) or not ents[i][2]:
            continue
        _, off, size, csize, method = ents[i]
        base = "%02d_%s" % (i, os.path.splitext(name)[0].lower())
        smk_path = os.path.join(smk_dir, base + ".smk")
        if not os.path.exists(smk_path):
            with open(smk_path, "wb") as f:
                f.write(decompress_entry(data, off, size, csize, method))
        frames_dir = os.path.join(png_root, base)
        if not os.path.exists(os.path.join(frames_dir, "info.txt")):
            decode(smk_path, frames_dir)
        if ffmpeg:
            os.makedirs(mp4_dir, exist_ok=True)
            make_mp4(ffmpeg, frames_dir, os.path.join(mp4_dir, base + ".mp4"))
        print("  %s" % base)
    if not ffmpeg:
        print("  (ffmpeg no encontrado: hay fotogramas PNG + WAV, falta el MP4)")


def make_mp4(ffmpeg, frames_dir, mp4):
    info = dict(line.strip().split("=", 1) for line in open(os.path.join(frames_dir, "info.txt")))
    tracks = [t for t in info.get("audio_tracks", "").split(",") if t]
    cmd = [ffmpeg, "-y", "-loglevel", "error", "-framerate", info["fps"],
           "-i", os.path.join(frames_dir, "frame_%05d.png")]
    # Track 0 is music/effects; INTRO adds voices on 1/2/3 (FR/DE/EN).
    voice = "3" if "3" in tracks else None
    for t in (["0"] if "0" in tracks else []) + ([voice] if voice else []):
        cmd += ["-i", os.path.join(frames_dir, "track%s.wav" % t)]
    n_audio = len(cmd[cmd.index("-i") + 2:]) // 2
    if n_audio == 2:
        cmd += ["-filter_complex", "[1:a][2:a]amix=inputs=2:normalize=0[a]", "-map", "0:v", "-map", "[a]"]
    elif n_audio == 1:
        cmd += ["-map", "0:v", "-map", "1:a"]
    cmd += ["-c:v", "libx264", "-crf", "12", "-preset", "slow", "-pix_fmt", "yuv420p",
            "-vf", "scale=iw*3:ih*3:flags=neighbor"]
    if n_audio:
        cmd += ["-c:a", "aac", "-b:a", "192k"]
    subprocess.run(cmd + [mp4], check=False)


def cmd_export(a):
    os.makedirs(a.out, exist_ok=True)
    if not a.skip_voices:
        print("Voces y guion:")
        export_voices(a.game, a.out, a.voice_lang.upper(), a.text_lang.upper(), a.voice_lang.upper())
    if not a.skip_videos:
        print("Videos:")
        export_videos(a.game, a.out, find_ffmpeg(a.ffmpeg))


# --- repack --------------------------------------------------------------

def cmd_repack(a):
    vox_dir = os.path.join(a.game, "VOX")
    os.makedirs(a.out, exist_ok=True)
    for bank in VOICE_BANKS:
        rec_dir = os.path.join(a.recordings, bank)
        if not os.path.isdir(rec_dir):
            continue
        src = os.path.join(vox_dir, "%s_%s.VOX" % (a.base_lang.upper(), bank.upper()))
        ents, data = entries(src)
        n = len(ents)
        recorded = {}
        for fn in os.listdir(rec_dir):
            stem, ext = os.path.splitext(fn)
            if ext.lower() != ".wav" or not stem.startswith(bank + "_"):
                continue
            try:
                recorded[int(stem.split("_")[1])] = os.path.join(rec_dir, fn)
            except ValueError:
                pass
        if not recorded:
            continue
        table = [0] * n
        body = bytearray()
        base = n * 4
        replaced = 0
        for slot, off, size, _, _ in ents[:-1]:
            if slot in recorded:
                samples, rate = read_any_wav(recorded[slot])
                samples = resample(samples, rate, GAME_RATE)
                if a.match_volume and size:
                    orig = vox_line_samples(data, off)
                    r_new, r_old = rms(samples), rms(orig)
                    if r_new > 0 and r_old > 0:
                        gain = r_old / r_new
                        peak = max(abs(s) for s in samples) or 1
                        gain = min(gain, 32000 / peak)
                        samples = [int(s * gain) for s in samples]
                blob = build_entry(samples)
                replaced += 1
            elif size:
                blob = raw_entry_span(data, off)
            else:
                continue
            table[slot] = base + len(body)
            body += blob
        table[n - 1] = base + len(body)
        missing = sorted(s for s in recorded if s >= n - 1)
        name = "%s_%s.VOX" % (a.prefix.upper(), bank.upper())
        with open(os.path.join(a.out, name), "wb") as f:
            f.write(struct.pack("<%dI" % n, *table) + body)
        print("  %s: %d frases nuevas%s" % (name, replaced,
              ("  (slots inexistentes ignorados: %s)" % missing) if missing else ""))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    e = sub.add_parser("export")
    e.add_argument("--game", required=True, help="carpeta Common del juego")
    e.add_argument("--out", required=True)
    e.add_argument("--voice-lang", default="EN", help="EN, FR o DE")
    e.add_argument("--text-lang", default="ES")
    e.add_argument("--ffmpeg", default=None)
    e.add_argument("--skip-voices", action="store_true")
    e.add_argument("--skip-videos", action="store_true")
    e.set_defaults(func=cmd_export)
    r = sub.add_parser("repack")
    r.add_argument("--game", required=True, help="carpeta Common del juego")
    r.add_argument("--recordings", required=True,
                   help="carpeta con subcarpetas gam/000/.../011 y archivos <banco>_<slot>.wav")
    r.add_argument("--out", required=True)
    r.add_argument("--base-lang", default="EN", help="VOX original de base para las frases sin grabar")
    r.add_argument("--prefix", default="EN", help="prefijo de los VOX nuevos (EN_ reemplaza el ingles)")
    r.add_argument("--no-match-volume", dest="match_volume", action="store_false",
                   help="no igualar el volumen al de la frase original")
    r.set_defaults(func=cmd_repack)
    a = ap.parse_args()
    a.func(a)


if __name__ == "__main__":
    main()
