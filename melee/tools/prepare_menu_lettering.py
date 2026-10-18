"""Recover the menu ribbon alphabet from the original option-name textures."""
import math
import struct

from PIL import Image, ImageDraw


def option_glyphs(archive, fallback):
    data = archive[32:]
    size, relocations, roots, references = struct.unpack_from('>4I', archive, 4)
    table = size + relocations * 4
    strings = table + (roots + references) * 8

    def word(offset):
        return struct.unpack_from('>I', data, offset)[0]

    symbols = {data[strings + word(table + i * 8 + 4):].split(b'\0', 1)[0]:
               word(table + i * 8) for i in range(roots)}

    def joints(node):
        while node:
            yield node
            yield from joints(word(node))
            node = word(node + 4)

    # MenMainCursor node 3 is the authored option lettering. Its texture table
    # is shared by the main, versus, single-player and options menus.
    node = list(joints(symbols[b'MenMainCursor_Top_matanim_joint']))[3]
    texture = word(word(node + 8) + 8)
    images = word(texture + 12)
    labels = {
        0: '1-PMode', 1: 'VS.Mode', 2: 'Trophies', 3: 'Options', 4: 'Data',
        5: 'RegularMatch', 6: 'EventMatch', 8: 'Stadium', 9: 'Training',
        10: 'Melee', 11: 'TournamentMelee', 12: 'SpecialMelee',
        13: 'CustomRules', 14: 'NameEntry', 15: 'Gallery', 16: 'Lottery',
        18: 'Collection', 19: 'Rumble', 20: 'Sound', 21: 'ScreenDisplay',
        24: 'EraseData', 25: 'Snapshots', 26: 'Archives', 27: 'SoundTest',
        28: 'MeleeRecords', 29: 'Special', 30: 'Classic', 31: 'Adventure',
        32: 'All-Star', 35: 'TargetTest', 36: 'Home-RunContest',
        37: 'Multi-ManMelee', 41: 'CameraMode', 42: 'StaminaMode',
        43: 'SuperSuddenDeath', 44: 'GiantMelee', 45: 'TinyMelee',
        46: 'InvisibleMelee', 47: 'Fixed-CameraMelee', 48: 'Single-ButtonMode',
        53: 'VS.Records', 54: 'BonusRecords', 55: 'Misc.Records',
        56: 'LightningMelee', 57: 'Slo-MoMelee',
    }
    glyphs = {}
    for index, text in labels.items():
        descriptor = word(images + index * 4)
        pixels, width, height, fmt = struct.unpack_from('>IHHI', data, descriptor)
        if fmt != 2 or (width, height) != (176, 30):
            raise ValueError('Unexpected menu option texture')
        image = Image.new('L', (width, height))
        offset = pixels
        for by in range(0, height, 4):
            for bx in range(0, width, 8):
                for y in range(4):
                    for x in range(8):
                        value = data[offset]
                        offset += 1
                        if bx + x < width and by + y < height:
                            image.putpixel((bx + x, by + y), (value & 15) * 17)
        # Read the white fill rather than the black outline, which connects
        # neighboring letters. Some compressed labels still touch: use another
        # label for those glyphs instead of guessing their boundaries.
        runs = []
        for x in range(width):
            if max(image.crop((x, 0, x + 1, height)).getdata()) <= 160:
                continue
            if not runs or x != runs[-1][-1] + 1:
                runs.append([x])
            else:
                runs[-1].append(x)
        if len(runs) != len(text):
            continue
        for char, run in zip(text, runs):
            glyph = image.crop((max(0, run[0] - 1), 3,
                                min(width, run[-1] + 2), 28))
            # Keep the least horizontally compressed source for each letter.
            if char not in glyphs or glyph.width > glyphs[char].width:
                glyphs[char] = glyph

    # These letters never occur in the source option names. Construct them from
    # the captured stroke width, cap height and round/diagonal forms.
    cap, baseline, stroke = 22, 21, 4
    if 'F' not in glyphs:
        glyphs['F'] = glyphs['E'].copy()
        ImageDraw.Draw(glyphs['F']).rectangle((5, 17, glyphs['F'].width, 25), fill=0)

    def lines(char, width, paths):
        im = Image.new('L', (width * 4, 25 * 4))
        draw = ImageDraw.Draw(im)
        for points in paths:
            draw.line([(x * 4, y * 4) for x, y in points], fill=255,
                      width=stroke * 4, joint='curve')
        glyphs[char] = im.resize((width, 25), Image.Resampling.LANCZOS)

    if 'Q' not in glyphs:
        glyphs['Q'] = glyphs['O'].copy()
        draw = ImageDraw.Draw(glyphs['Q'])
        w = glyphs['Q'].width
        draw.line((w * .55, 15, w - 1, 24), fill=255, width=stroke)
    lines('J', 14, [[(11, 1), (11, 17), (9, 20), (5, 21), (2, 19), (1, 16)]])
    lines('K', 18, [[(2, 1), (2, baseline)], [(15, 1), (3, 12), (16, baseline)]])
    lines('U', 19, [[(2, 1), (2, 16), (4, 20), (9, 21), (14, 20), (16, 16), (16, 1)]])
    lines('W', 25, [[(2, 1), (7, 21), (12, 5), (17, 21), (22, 1)]])
    lines('X', 19, [[(2, 1), (16, 21)], [(16, 1), (2, 21)]])
    lines('Y', 19, [[(2, 1), (9, 12), (16, 1)], [(9, 12), (9, 21)]])
    lines('Z', 18, [[(2, 1), (15, 1), (2, 21), (15, 21)]])
    lines('k', 15, [[(2, 1), (2, 21)], [(12, 7), (3, 15), (13, 21)]])
    lines('w', 23, [[(2, 7), (6, 21), (11, 9), (16, 21), (20, 7)]])
    lines('x', 15, [[(2, 7), (12, 21)], [(12, 7), (2, 21)]])
    lines('z', 14, [[(2, 8), (11, 8), (2, 21), (11, 21)]])
    if 'f' not in glyphs:
        lines('f', 12, [[(10, 1), (7, 1), (5, 4), (5, 21)], [(1, 8), (10, 8)]])
    if 'j' not in glyphs:
        lines('j', 9, [[(6, 8), (6, 22), (4, 24), (1, 24)], [(6, 1), (6, 3)]])
    if 'q' not in glyphs:
        glyphs['q'] = glyphs['p'].transpose(Image.Transpose.FLIP_LEFT_RIGHT)
    missing = set('ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz') - glyphs.keys()
    if missing:
        raise ValueError('Missing menu option letters: ' + str(missing))
    # Punctuation and digits beyond the original labels use the supplied notice
    # family. Every alphabetic label uses captured or reconstructed menu glyphs.
    for char, glyph in fallback.items():
        if char not in glyphs:
            glyphs[char] = glyph.resize((max(1, math.ceil(glyph.width * 22 / glyph.height)), 25),
                                        Image.Resampling.LANCZOS)
    glyphs[' '] = Image.new('L', (8, 25))
    return glyphs, cap
