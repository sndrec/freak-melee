"""Accented Latin lettering and punctuation for the context-specific GX fonts."""
import math
import unicodedata

from PIL import Image, ImageDraw, ImageFont


EXTRA_CHARACTERS = (''.join(map(chr, range(0xA0, 0x180))) +
                    '\u2010\u2011\u2012\u2013\u2014\u2018\u2019\u201a\u201c\u201d\u201e\u2022\u2026\u2032\u2033\u20ac\u2122\ufffd')


def raster_extended(path, size, tracking, cap=None):
    font = ImageFont.truetype(str(path), size)
    baseline = -font.getbbox('H', anchor='ls')[1]
    bottom = max(font.getbbox(chr(c), anchor='ls')[3] for c in range(32, 127))
    missing = font.getmask(chr(0x10FFFF))
    missing = (missing.size, bytes(missing))
    result = {}
    for c in EXTRA_CHARACTERS:
        mask = font.getmask(c)
        if (mask.size, bytes(mask)) == missing:
            continue
        left, top, right, last = font.getbbox(c, anchor='ls')
        top = min(0, baseline + top)
        origin = -min(0, left)
        width = max(1, math.ceil(font.getlength(c)) + origin, right + origin)
        image = Image.new('L', (width, max(baseline + bottom, baseline + last) - top))
        ImageDraw.Draw(image).text((origin, baseline - top), c, font=font,
                                  anchor='ls', fill=255)
        if cap:
            scale = cap / baseline
            image = image.resize((max(1, round(image.width * scale)),
                                  max(1, round(image.height * scale))), Image.Resampling.LANCZOS)
            top = round(top * scale)
        result[c] = (image, 0, top, image.width + tracking)
    return result


def accent(base, mark, cap):
    """Build an accent in the source letter's proportions, above its cap line."""
    height = max(4, round(cap * .25))
    width = max(7, round(cap * .45))
    stroke = max(1, round(cap / 12))
    upper = Image.new('L', (width * 4, height * 4))
    draw = ImageDraw.Draw(upper)
    x, y, w, h = 2, 1, width * 4 - 4, height * 4 - 3
    thickness = stroke * 4
    if mark in ('\u0300', '\u0301'):
        points = [(x, y), (w, h)] if mark == '\u0300' else [(x, h), (w, y)]
        draw.line(points, fill=255, width=thickness)
    elif mark in ('\u0302', '\u030c'):
        points = [(x, h), (width * 2, y), (w, h)]
        if mark == '\u030c':
            points = [(a, height * 4 - b) for a, b in points]
        draw.line(points, fill=255, width=thickness)
    elif mark == '\u0303':
        draw.line([(x, height * 2), (width, y), (width * 3, h), (w, height * 2)],
                  fill=255, width=thickness, joint='curve')
    elif mark == '\u0304':
        draw.line([(x, height * 2), (w, height * 2)], fill=255, width=thickness)
    elif mark == '\u0306':
        draw.arc((x, -height * 2, w, h), 0, 180, fill=255, width=thickness)
    elif mark in ('\u0307', '\u0308'):
        centers = [width * 2] if mark == '\u0307' else [width, width * 3]
        for center in centers:
            draw.ellipse((center - thickness / 2, y, center + thickness / 2,
                          y + thickness), fill=255)
    elif mark == '\u030a':
        draw.ellipse((width, y, width * 3, h), outline=255, width=thickness)
    elif mark == '\u030b':
        for start in [x, width * 2]:
            draw.line([(start, h), (start + width, y)], fill=255, width=thickness)
    elif mark in ('\u0327', '\u0328'):
        draw.line([(width * 2, y), (width, height * 2), (width * 2, h), (w, h)],
                  fill=255, width=thickness, joint='curve')
    else:
        return None
    upper = upper.resize((width, height), Image.Resampling.LANCZOS)
    below = mark in ('\u0327', '\u0328')
    margin = height + 1
    image = Image.new('L', (max(base.width, width), base.height + margin))
    left = (image.width - base.width) // 2
    image.paste(base, (left, 0 if below else margin))
    image.paste(upper, ((image.width - width) // 2, base.height if below else 0))
    return image, -left, 0 if below else -margin


def expand(glyphs, cap, tracking, caps, font_path=None, font_size=24, raster=False):
    result = {}
    for code in range(32, 127):
        c = chr(code)
        image = glyphs[c.upper() if caps and c.isalpha() else c]
        result[c] = (image, 0, 0, image.width + tracking)
    fallback = raster_extended(font_path, font_size, tracking, None if raster else cap) if font_path else {}
    # A real font supplies its own accented forms. Disc-derived faces compose
    # accents over their own letters; only missing non-decomposable forms use Folk.
    for c in EXTRA_CHARACTERS:
        target = c.upper() if caps else c
        decomposition = unicodedata.normalize('NFD', target)
        composed = None
        if not raster or target not in fallback:
            ligature = {'Æ': 'AE', 'æ': 'ae', 'Œ': 'OE', 'œ': 'oe'}.get(target, target)
            if len(ligature) > 1 and all(letter in glyphs for letter in ligature):
                pieces = [glyphs[letter] for letter in ligature]
                image = Image.new('L', (sum(p.width for p in pieces), max(p.height for p in pieces)))
                x = 0
                for piece in pieces:
                    image.paste(piece, (x, 0)); x += piece.width
                composed = image, 0, 0, image.width + tracking
            elif len(decomposition) == 2 and decomposition[0] in glyphs:
                base = glyphs[decomposition[0]]
                marked = accent(base, decomposition[1], cap)
                if marked:
                    composed = (*marked, base.width + tracking)
            elif target in glyphs:
                image = glyphs[target]
                composed = image, 0, 0, image.width + tracking
            else:
                struck = {'Ø': 'O', 'ø': 'o', 'Ł': 'L', 'ł': 'l',
                          'Ð': 'D', 'ð': 'd', 'Đ': 'D', 'đ': 'd',
                          'Ħ': 'H', 'ħ': 'h', 'Ŧ': 'T', 'ŧ': 't'}
                if target in struck and struck[target] in glyphs:
                    image = glyphs[struck[target]].copy()
                    draw = ImageDraw.Draw(image)
                    points = [(0, image.height * .65), (image.width - 1, image.height * .3)]
                    if target not in 'ØøŁł':
                        points = [(0, image.height * .4), (image.width - 1, image.height * .4)]
                    draw.line(points, fill=255, width=max(1, round(cap / 12)))
                    composed = image, 0, 0, image.width + tracking
        if composed:
            result[c] = composed
        elif target in fallback:
            result[c] = fallback[target]
    # Spaces and common punctuation reuse the context's own shapes.
    aliases = {'\u00a0': ' ', '\u2010': '-', '\u2011': '-', '\u2012': '-',
               '\u2018': "'", '\u2019': "'", '\u201a': ',',
               '\u201c': '"', '\u201d': '"', '\u201e': '"',
               '\u2032': "'", '\u2033': '"'}
    for c, source in aliases.items():
        result.setdefault(c, result[source])
    for c, count in [('\u2013', 2), ('\u2014', 3), ('\u2026', 3)]:
        source = result['.' if c == '\u2026' else '-']
        piece, left, top, step = source
        image = Image.new('L', (step * (count - 1) + piece.width, piece.height))
        for index in range(count):
            image.paste(piece, (index * step, 0))
        result.setdefault(c, (image, left, top, step * count))
    result.setdefault('\ufffd', result['?'])
    compositions = []
    for c in sorted(result):
        pair = unicodedata.decomposition(c).split()
        if len(pair) == 2 and not pair[0].startswith('<'):
            base, mark = map(lambda value: int(value, 16), pair)
            if chr(base) in result:
                compositions.append((base, mark, ord(c)))
    return result, sorted(compositions)
