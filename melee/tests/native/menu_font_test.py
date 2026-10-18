"""Check generated GX font records, texture encoding and accented-name coverage."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from PIL import Image


def verify(path, baseline):
    raw = path.read_bytes()
    width, height, cap, count, fmt, pairs = struct.unpack_from('<6H', raw)
    assert width == 1024 and 0 < height <= 1024 and height % 8 == 0
    assert cap and 190 <= count <= 512 and pairs <= 256 and fmt in (0, 2)
    glyphs = list(struct.iter_unpack('<I5H2hH', raw[12:12 + count * 20]))
    assert [g[0] for g in glyphs] == sorted(set(g[0] for g in glyphs))
    by_code = {g[0]: g for g in glyphs}
    for c in 'éÉöÖñÑÅåüÜÇçČčŠšŽžŁłŒœÆæß—…’“”\ufffd':
        assert ord(c) in by_code, (path, ascii(c))
    image = Image.open(path.with_suffix('.png'))
    assert image.size == (width, height)
    for index, glyph in enumerate(glyphs):
        code, x, y, w, h, advance, left, top, reserved = glyph
        assert 0 < w <= 128 and 0 < h <= 96 and advance <= 160
        assert x + w <= width and y + h <= height and reserved == 0
        assert -64 <= left <= 64 and -64 <= top <= 64
        if code < 127 and baseline:
            old_metrics, old_ink = baseline[index]
            assert [w, h, advance] == old_metrics, (path, code)
            assert hashlib.sha256(image.crop((x, y, x+w, y+h)).tobytes()).hexdigest() == old_ink, (path, code)
    start = 12 + count * 20
    compositions = list(struct.iter_unpack('<3I', raw[start:start + pairs * 12]))
    assert compositions == sorted(set(compositions))
    lookup = {(base, mark): combined for base, mark, combined in compositions}
    for a, b in [('é', 'e\u0301'), ('Å', 'A\u030a'), ('č', 'c\u030c')]:
        assert lookup[ord(b[0]), ord(b[1])] == ord(a)
    for base, mark, combined in compositions:
        assert base in by_code and combined in by_code and 0x300 <= mark <= 0x36f
    pixels = raw[start + pairs * 12:]
    assert len(pixels) == width * height // (2 if fmt == 0 else 1)
    # Compare the tiled GPU bytes with the inspection PNG, including both fill
    # and coverage for the outlined face. Check every texel, not just glyph ink.
    offset = 0
    rows = 8 if fmt == 0 else 4
    for by in range(0, height, rows):
        for bx in range(0, width, 8):
            for y in range(rows):
                for x in range(8):
                    index = y * 8 + x
                    byte = pixels[offset + (index // 2 if fmt == 0 else index)]
                    if fmt == 0:
                        value = (byte >> (4 if index % 2 == 0 else 0)) & 15
                        assert value == image.getpixel((bx+x, by+y)) >> 4
                    else:
                        fill, alpha = image.getpixel((bx+x, by+y))
                        assert byte == (alpha >> 4) * 16 + (fill >> 4)
            offset += 32
    print(f'{path.name}: {count} glyphs, {pairs} accent pairs, {width}x{height}', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fonts', type=Path, required=True)
    parser.add_argument('--ascii-baseline', type=Path)
    args = parser.parse_args()
    baseline = json.loads(args.ascii_baseline.read_text()) if args.ascii_baseline else {}
    names = ['menu-heading', 'notice', 'css-tile', 'menu-option', 'menu-caption', 'result-name', 'result-winner']
    for name in names:
        path = args.fonts / (name + '.bin')
        verify(path, baseline.get(path.name))
    print('All seven font atlases passed.', flush=True)


if __name__ == '__main__':
    main()
