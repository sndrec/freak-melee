"""Recover the original HSD text face from a verified Melee executable."""
from pathlib import Path
import re

from PIL import Image

from prepare_native_assets import extract_dol, extract_symbol


def decode_i4(raw, width, height):
    if width % 8 or height % 8 or len(raw) < width * height // 2:
        raise ValueError('Invalid tiled I4 font image')
    image = Image.new('L', (width, height))
    offset = 0
    for by in range(0, height, 8):
        for bx in range(0, width, 8):
            for y in range(8):
                for x in range(8):
                    index = y * 8 + x
                    nibble = raw[offset + index // 2] >> (4 if index % 2 == 0 else 0)
                    image.putpixel((bx + x, by + y), (nibble & 15) * 17)
            offset += 32
    return image


def text_glyphs(iso, cap_height=24):
    root = Path(__file__).resolve().parents[1]
    expected = re.search(r'^hash: ([0-9a-f]{40})$',
                         (root/'config/GALE01/config.yml').read_text(), re.MULTILINE).group(1)
    dol, sections = extract_dol(iso, expected)
    symbols = (root/'config/GALE01/symbols.txt').read_text()
    pixels = extract_symbol(dol, sections, symbols, 'HSD_SisLib_FontAtlas')
    bearings = extract_symbol(dol, sections, symbols, 'HSD_SisLib_8040CB00')
    if len(pixels) != 287 * 512 or len(bearings) < 287 * 2:
        raise ValueError('Unexpected original HSD font layout')
    indices = {c: i for i, c in enumerate('0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz')}
    indices.update(dict(zip(',.:;?!^_-/~|\'"()[]{}+=<>',
                            [230,231,233,234,235,236,237,238,252,240,241,242,243,244,245,246,247,248,249,250,251,254,255,256])))
    indices.update({'$':258, '%':259, '#':260, '&':261, '*':262, '@':263,
                    '¥':257, '×':253, '·':232, '—':239})
    images = [decode_i4(pixels[i*512:(i+1)*512], 32, 32) for i in range(287)]
    cap = images[indices['H']].getbbox()
    scale = cap_height / (cap[3] - cap[1])
    glyphs = {}
    for char, index in indices.items():
        left, right = bearings[index*2:index*2+2]
        # Retail kerning retains one pixel on each side of the visible span.
        image = images[index].crop((max(0, left-1), cap[1], min(32, 33-right), 32))
        glyphs[char] = image.resize((max(1, round(image.width*scale)),
                                    max(1, round(image.height*scale))), Image.Resampling.LANCZOS)
    glyphs[' '] = Image.new('L', (round(16*scale), round((32-cap[1])*scale)))
    glyphs['\\'] = glyphs['/'].transpose(Image.Transpose.FLIP_LEFT_RIGHT)
    glyphs['`'] = glyphs["'"].transpose(Image.Transpose.FLIP_LEFT_RIGHT)
    return glyphs
