"""Recover the ribbon and caption alphabets from the original menu textures."""
import math
import struct

from PIL import Image, ImageDraw


class MenuArchive:
    def __init__(self, archive):
        self.data = archive[32:]
        size, relocations, roots, references = struct.unpack_from('>4I', archive, 4)
        table = size + relocations * 4
        strings = table + (roots + references) * 8
        self.symbols = {self.data[strings + self.word(table + i * 8 + 4):].split(b'\0', 1)[0].decode():
                        self.word(table + i * 8) for i in range(roots)}

    def word(self, offset):
        return struct.unpack_from('>I', self.data, offset)[0]

    def nodes(self, name):
        child, sibling = (0, 4) if name.endswith('_matanim_joint') else (8, 12)
        def visit(node):
            while node:
                yield node
                yield from visit(self.word(node + child))
                node = self.word(node + sibling)
        return list(visit(self.symbols[name]))

    def image(self, descriptor, coverage=False):
        pixels, width, height, fmt = struct.unpack_from('>IHHI', self.data, descriptor)
        if fmt not in (0, 2):
            raise ValueError('Expected I4 or IA4 menu lettering')
        image = Image.new('L', (width, height))
        offset = pixels
        rows = 8 if fmt == 0 else 4
        for by in range(0, height, rows):
            for bx in range(0, width, 8):
                for y in range(rows):
                    for x in range(8):
                        i = y * 8 + x
                        value = self.data[offset + (i // 2 if fmt == 0 else i)]
                        if fmt == 0:
                            value >>= 4 if i % 2 == 0 else 0
                        elif coverage:
                            value = ((value & 15) * (value >> 4) + 7) // 15
                        if bx + x < width and by + y < height:
                            image.putpixel((bx + x, by + y), (value & 15) * 17)
                offset += 32
        return image

    def animated_images(self, name, index):
        node = self.nodes(name)[index]
        texture = self.word(self.word(node + 8) + 8)
        images = self.word(texture + 12)
        count = struct.unpack_from('>H', self.data, texture + 20)[0]
        return [self.image(self.word(images + n * 4)) for n in range(count)]

    def joint_image(self, name, index, coverage=False):
        joint = self.nodes(name)[index]
        dobj = self.word(joint + 16)
        material = self.word(dobj + 8)
        texture = self.word(material + 8)
        return self.image(self.word(texture + 76), coverage)


def letter_runs(image, threshold):
    runs = []
    for x in range(image.width):
        if max(image.crop((x, 0, x + 1, image.height)).getdata()) <= threshold:
            continue
        if not runs or x != runs[-1][-1] + 1:
            runs.append([x])
        else:
            runs[-1].append(x)
    return runs


def align_alphabet_baseline(glyphs, baseline):
    # The original labels were rasterized at slightly different positions.
    # Recovered letters share a baseline; descenders and Q retain their tails.
    for char in 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz':
        if char in 'Qgjpqy':
            continue
        glyph = glyphs[char]
        bounds = glyph.point(lambda value: 255 if value > 160 else 0).getbbox()
        if bounds:
            aligned = Image.new('L', glyph.size)
            aligned.paste(glyph, (0, baseline - bounds[3]))
            glyphs[char] = aligned


def option_glyphs(archive, fallback):
    source = MenuArchive(archive)

    # MenMainCursor node 3 is the authored option lettering. Its texture table
    # is shared by the main, versus, single-player and options menus.
    images = source.animated_images('MenMainCursor_Top_matanim_joint', 3)
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
        image = images[index]
        width, height = image.size
        if (width, height) != (176, 30):
            raise ValueError('Unexpected menu option texture')
        # Read the white fill rather than the black outline, which connects
        # neighboring letters. Some compressed labels still touch: use another
        # label for those glyphs instead of guessing their boundaries.
        runs = letter_runs(image, 160)
        if len(runs) != len(text):
            continue
        for char, run in zip(text, runs):
            glyph = image.crop((max(0, run[0] - 1), 3,
                                min(width, run[-1] + 2), 28))
            # Keep the least horizontally compressed source for each letter.
            if char not in glyphs or glyph.width > glyphs[char].width:
                glyphs[char] = glyph

    complete_alphabet(glyphs, fallback, 22)
    align_alphabet_baseline(glyphs, 22)
    return glyphs, 22


def complete_alphabet(glyphs, fallback, cap):
    # Construct absent letters from the face's cap height and captured forms.
    ratio, stroke = cap / 22, 4
    if 'F' not in glyphs:
        glyphs['F'] = glyphs['E'].copy()
        ImageDraw.Draw(glyphs['F']).rectangle((5 * ratio, 17 * ratio, glyphs['F'].width, 25 * ratio), fill=0)

    def lines(char, width, paths):
        if char in glyphs:
            return
        width = math.ceil(width * ratio)
        height = math.ceil(25 * ratio)
        im = Image.new('L', (width * 4, height * 4))
        draw = ImageDraw.Draw(im)
        for points in paths:
            draw.line([(x * 4 * ratio, y * 4 * ratio) for x, y in points], fill=255,
                      width=round(stroke * 4 * ratio), joint='curve')
        glyphs[char] = im.resize((width, height), Image.Resampling.LANCZOS)

    if 'Q' not in glyphs:
        glyphs['Q'] = glyphs['O'].copy()
        draw = ImageDraw.Draw(glyphs['Q'])
        w = glyphs['Q'].width
        draw.line((w * .55, 15 * ratio, w - 1, 24 * ratio), fill=255, width=round(stroke * ratio))
    lines('J', 14, [[(11, 1), (11, 17), (9, 20), (5, 21), (2, 19), (1, 16)]])
    lines('K', 18, [[(2, 1), (2, 21)], [(15, 1), (3, 12), (16, 21)]])
    lines('U', 19, [[(2, 1), (2, 16), (4, 20), (9, 21), (14, 20), (16, 16), (16, 1)]])
    lines('W', 25, [[(2, 1), (7, 21), (12, 5), (17, 21), (22, 1)]])
    lines('X', 19, [[(2, 1), (16, 21)], [(16, 1), (2, 21)]])
    lines('Y', 19, [[(2, 1), (9, 12), (16, 1)], [(9, 12), (9, 21)]])
    lines('Z', 18, [[(2, 1), (15, 1), (2, 21), (15, 21)]])
    lines('H', 19, [[(2, 1), (2, 21)], [(16, 1), (16, 21)], [(2, 11), (16, 11)]])
    lines('L', 16, [[(2, 1), (2, 21), (14, 21)]])
    if 'G' not in glyphs:
        glyphs['G'] = glyphs['C'].copy()
        draw = ImageDraw.Draw(glyphs['G'])
        w = glyphs['G'].width
        draw.line((w - 2, 20 * ratio, w - 2, 12 * ratio, w * .55, 12 * ratio),
                  fill=255, width=round(stroke * ratio))
    if 'B' not in glyphs:
        glyphs['B'] = glyphs['P'].copy()
        upper = glyphs['P'].crop((0, 0, glyphs['P'].width, math.ceil(cap * .55)))
        glyphs['B'].paste(upper.transpose(Image.Transpose.FLIP_TOP_BOTTOM), (0, math.floor(cap * .45)))
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
    # Punctuation and digits beyond the original labels use the original HSD
    # text face. Alphabetic labels keep captured or reconstructed menu glyphs.
    for char, glyph in fallback.items():
        if char not in glyphs:
            glyphs[char] = glyph.resize((max(1, math.ceil(glyph.width * cap / glyph.height)), math.ceil(cap + 3)),
                                        Image.Resampling.LANCZOS)
    glyphs[' '] = Image.new('L', (round(8 * ratio), math.ceil(cap + 3)))


def caption_glyphs(archive, fallback):
    source = MenuArchive(archive)
    names = {
        82: {2: 'ItemSwitch', 3: 'AdditionalRules', 4: 'Settei', 5: 'Kikakusyo',
             6: 'EndPictures', 7: 'NINTENDOTitle', 8: 'VS.Records',
             9: 'BonusRecords', 10: 'Misc.Records'},
        84: {0: '1-PMode', 1: 'VS.Mode', 2: 'Trophies', 3: 'Options', 4: 'Data'},
        85: {0: 'RegularMatch', 1: 'EventMatch', 3: 'Stadium', 4: 'Training',
             5: 'Training', 6: 'TournamentMelee', 7: 'SpecialMelee',
             8: 'CustomRules', 9: 'NameEntry', 10: 'Rumble', 11: 'Sound',
             12: 'ScreenDisplay', 13: 'Deflicker', 15: 'EraseData',
             16: 'Snapshots', 17: 'Archives', 18: 'SoundTest',
             19: 'MeleeRecords', 20: 'Special'},
    }
    samples = [(source.joint_image('MenMainPanel_Top_joint', 86), 'MainMenu')]
    for index, labels in names.items():
        images = source.animated_images('MenMainPanel_Top_matanim_joint', index)
        samples.extend((images[n], text) for n, text in labels.items())
    glyphs = {}
    for image, text in samples:
        # Remove the shared italic shear before finding letter boundaries.
        # Reapply it at drawing time, preserving a common baseline for strings.
        for slope in [.30, .35, .25, .40, .20, .45]:
            upright = image.transform((image.width + 16, image.height),
                Image.Transform.AFFINE, (1, -slope, 0, 0, 1, 0),
                Image.Resampling.BICUBIC)
            runs = letter_runs(upright, 96)
            if len(runs) != len(text):
                continue
            for char, run in zip(text, runs):
                glyph = upright.crop((max(0, run[0] - 1), 0, run[-1] + 2, 28))
                if char not in glyphs or glyph.width > glyphs[char].width:
                    glyphs[char] = glyph
            break
    complete_alphabet(glyphs, fallback, 24)
    align_alphabet_baseline(glyphs, 24)
    return glyphs, 24
