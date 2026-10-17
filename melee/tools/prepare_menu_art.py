"""Extract starter-fighter portraits from the user's Melee disc.

The CSS joints and texture descriptors select the artwork. Generated PNGs stay
in the build directory; the repository contains no disc-derived artwork.
"""
import argparse
import re
import struct
from pathlib import Path

from PIL import Image
from prepare_menu_fonts import disc_file
from prepare_native_assets import extract_dol


def portraits(iso, source):
    config = (source / 'config/GALE01/config.yml').read_text()
    extract_dol(iso, re.search(r'hash: ([0-9a-f]{40})', config).group(1))
    data = disc_file(iso, 'MnSlChr.usd')
    size, relocations, roots, references = struct.unpack_from('>4I', data, 4)
    root_table = 32 + size + relocations * 4
    strings = root_table + (roots + references) * 8

    def word(offset):
        if offset < 0 or offset + 4 > size:
            raise ValueError('CSS descriptor outside the DAT data section')
        return struct.unpack_from('>I', data, 32 + offset)[0]

    table = None
    for index in range(roots):
        offset, name = struct.unpack_from('>II', data, root_table + index * 8)
        if data[strings + name:].split(b'\0', 1)[0] == b'MnSelectChrDataTable':
            table = offset
    if table is None:
        raise ValueError('CSS model table is missing')
    joints, visited = [], set()

    def visit(joint):
        while joint:
            if joint in visited or len(joints) >= 1024:
                raise ValueError('Invalid CSS joint hierarchy')
            visited.add(joint)
            joints.append(joint)
            visit(word(joint + 8))
            joint = word(joint + 12)

    # Models: background, hand, token, menu. Joint IDs match lb_80011E24's
    # depth-first indexing; use the same named constants as the runtime CSS.
    visit(word(table + 16 + 3 * 16))
    definitions = (source / 'src/melee/mn/types.h').read_text()
    icon_ids = dict((name.lower(), int(value, 16)) for name, value in re.findall(
        r'ICONJOINT_(\w+)\s*=\s*0x([\da-fA-F]+)', definitions))
    kinds = re.findall(r'/\* ([\dA-F]{2}) \*/ Ft_Kind_(\w+),',
                       (source / 'src/melee/ft/forward.h').read_text())
    aliases = {'popo': 'poponana', 'nana': 'poponana', 'seak': 'zelda',
               'marth': 'mars', 'roy': 'emblem'}
    for number, kind in kinds:
        icon = icon_ids.get(aliases.get(kind.lower(), kind.lower()))
        if icon is None:
            continue
        candidates = []
        dobj = word(joints[icon] + 16)
        while dobj:
            material = word(dobj + 8)
            texture = word(material + 8) if material else 0
            while texture:
                image, palette = word(texture + 76), word(texture + 80)
                if image and palette and word(image + 8) == 9:
                    candidates.append((image, palette))
                texture = word(texture + 4)
            dobj = word(dobj + 4)
        if len(candidates) != 1:
            raise ValueError('Expected one indexed CSS portrait for ' + kind)
        image, palette = candidates[0]
        pixels, width, height, fmt = struct.unpack_from('>IHHI', data, 32 + image)
        if (width, height, fmt, word(palette + 4)) != (64, 56, 9, 2):
            raise ValueError('Unexpected CSS portrait format for ' + kind)
        colors = []
        palette_pixels = word(palette)
        entries = struct.unpack_from('>H', data, 32 + palette + 12)[0]
        for index in range(entries):
            value = struct.unpack_from('>H', data, 32 + palette_pixels + index * 2)[0]
            if value & 0x8000:
                color = ((value >> 10 & 31) * 255 // 31,
                         (value >> 5 & 31) * 255 // 31,
                         (value & 31) * 255 // 31, 255)
            else:
                color = ((value >> 8 & 15) * 17, (value >> 4 & 15) * 17,
                         (value & 15) * 17, (value >> 12 & 7) * 255 // 7)
            colors.append(color)
        result = Image.new('RGBA', (width, height))
        position = 32 + pixels
        for by in range(0, height, 4):
            for bx in range(0, width, 8):
                for y in range(4):
                    for x in range(8):
                        result.putpixel((bx + x, by + y),
                                        colors[data[position + y * 8 + x]])
                position += 32
        # Exclude the baked border and stock name. Clones draw their own names
        # using the user's angular glyph set, including after a rename.
        yield int(number, 16), result.crop((3, 3, 61, 41))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--iso', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    for kind, image in portraits(args.iso, Path(__file__).resolve().parents[1]):
        image.save(args.output / f'stock-{kind}.png')
