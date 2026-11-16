"""Copy each starter fighter's stock icon and HUD emblem from IfAll.usd."""
import re
import struct

from PIL import Image
from prepare_menu_fonts import disc_file


def write_hud_art(iso, output, source):
    from prepare_menu_art import indexed_image
    data = disc_file(iso, 'IfAll.usd')
    size, relocations, roots, references = struct.unpack_from('>4I', data, 4)
    root_table = 32 + size + relocations * 4
    strings = root_table + (roots + references) * 8

    def word(offset):
        if not 0 <= offset <= size - 4:
            raise ValueError('HUD descriptor outside DAT data')
        return struct.unpack_from('>I', data, 32 + offset)[0]

    symbols = {}
    for index in range(roots):
        offset, name = struct.unpack_from('>II', data, root_table + index * 8)
        symbols[data[strings + name:].split(b'\0', 1)[0]] = offset

    def texture(root):
        descriptor = word(symbols[root])
        node = word(word(descriptor + 8))
        # Both original HUD models put their image on the first child.
        return word(word(word(node) + 8) + 8)

    stock = texture(b'Stc_scemdls')
    emblem = texture(b'DmgMrk_scene_models')
    fobj = word(word(emblem + 8) + 8)
    length, start = word(fobj + 4), word(fobj + 16)
    encoded = data[32 + start:32 + start + length]
    # The emblem's original texture animation is one constant-key packet,
    # with unsigned fixed-point values and one-frame waits for roster entries.
    if encoded[:2] != b'\xe1\x03' or data[32 + fobj + 13] != 0x84:
        raise ValueError('Unexpected HUD emblem animation encoding')
    emblem_frames = []
    for offset in range(2, 2 + 26 * 2, 2):
        value, wait = encoded[offset:offset + 2]
        if wait != 1 or value % 16:
            raise ValueError('Unexpected HUD emblem key')
        emblem_frames.append(value // 16)

    definitions = (source / 'src/melee/ft/forward.h').read_text()
    fighters = re.findall(r'/\* ([\dA-F]{2}) \*/ Ft_Kind_(\w+),', definitions)
    characters = {name.lower(): int(number, 16) for number, name in
                  re.findall(r'/\* ([\dA-F]{2}) \*/ CKind_(\w+),', definitions)}
    aliases = {'popo': 'poponana', 'nana': 'poponana'}
    for number, name in fighters:
        kind = int(number, 16)
        if kind > 26:
            continue
        character = characters[aliases.get(name.lower(), name.lower())]
        frame = 25 if name == 'Seak' else character - (character > 19)
        image = word(word(stock + 12) + frame * 4)
        palette = word(word(stock + 16) + frame * 4)
        indexed_image(data, image, palette).save(output / f'hud-stock-{kind}.png')
        image = word(word(emblem + 12) + emblem_frames[frame] * 4)
        pixels, width, height, fmt = struct.unpack_from('>IHHI', data, 32 + image)
        if fmt != 0 or width % 8 or height % 8:
            raise ValueError('Unexpected HUD emblem image format')
        result = Image.new('RGBA', (width, height))
        position = 32 + pixels
        for by in range(0, height, 8):
            for bx in range(0, width, 8):
                for y in range(8):
                    for x in range(8):
                        value = data[position + y * 4 + x // 2]
                        alpha = ((value >> (4 if x % 2 == 0 else 0)) & 15) * 17
                        result.putpixel((bx + x, by + y), (255, 255, 255, alpha))
                position += 32
        result.save(output / f'hud-emblem-{kind}.png')
