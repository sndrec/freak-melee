"""Preserve the copied face details while recoloring its CMPR and C8 textures."""

import struct
from PIL import Image


def rgb565(value):
    return (
        (value >> 11) * 255 // 31,
        ((value >> 5) & 63) * 255 // 63,
        (value & 31) * 255 // 31,
    )


def decode(dat, texture):
    image = dat.u32(texture + 76)
    offset, width, height, fmt = struct.unpack_from(">IHHI", dat.data, image)
    result = Image.new("RGBA", (width, height))
    if fmt == 14:
        for by in range(0, height, 8):
            for bx in range(0, width, 8):
                for ox, oy in ((0, 0), (4, 0), (0, 4), (4, 4)):
                    a, b, bits = struct.unpack_from(">HHI", dat.data, offset)
                    offset += 8
                    ca, cb = rgb565(a), rgb565(b)
                    colors = [(*ca, 255), (*cb, 255)]
                    if a > b:
                        colors += [
                            tuple((2 * ca[k] + cb[k]) // 3 for k in range(3)) + (255,),
                            tuple((ca[k] + 2 * cb[k]) // 3 for k in range(3)) + (255,),
                        ]
                    else:
                        colors += [
                            tuple((ca[k] + cb[k]) // 2 for k in range(3)) + (255,),
                            (0, 0, 0, 0),
                        ]
                    for y in range(4):
                        for x in range(4):
                            xx, yy = bx + ox + x, by + oy + y
                            if xx < width and yy < height:
                                result.putpixel(
                                    (xx, yy),
                                    colors[(bits >> (30 - 2 * (y * 4 + x))) & 3],
                                )
    elif fmt == 9:
        tlut = dat.u32(texture + 80)
        palette = dat.u32(tlut)
        if dat.u32(tlut + 4) != 1:
            raise ValueError("Expected the copied rig's RGB5A3 palette")
        for by in range(0, height, 4):
            for bx in range(0, width, 8):
                for y in range(4):
                    for x in range(8):
                        index = dat.data[offset]
                        offset += 1
                        value = struct.unpack_from(">H", dat.data, palette + index * 2)[
                            0
                        ]
                        color = (
                            (
                                ((value >> 10) & 31) * 255 // 31,
                                ((value >> 5) & 31) * 255 // 31,
                                (value & 31) * 255 // 31,
                                255,
                            )
                            if value & 0x8000
                            else (
                                ((value >> 8) & 15) * 17,
                                ((value >> 4) & 15) * 17,
                                (value & 15) * 17,
                                ((value >> 12) & 7) * 255 // 7,
                            )
                        )
                        result.putpixel((bx + x, by + y), color)
    else:
        raise ValueError(f"Unexpected copied texture format {fmt}")
    return result


def recolor(costume, output):
    mesh = costume.u32(next(iter(costume.roots.values())) + 16)
    material = 0
    while mesh:
        mobj = costume.u32(mesh + 8)
        texture = costume.u32(mobj + 8)
        image = decode(costume, texture)
        pixels = []
        for r, g, b, a in image.getdata():
            # Pink/red skin becomes teal/brass; neutral face details survive.
            if r > g * 1.08 and r > b * 0.9:
                if material not in (3, 13):
                    color = (int(g * 0.32), int(r * 0.73), int(r * 0.66), a)
                else:
                    color = (int(r * 0.92), int(r * 0.69), int(r * 0.25), a)
            else:
                color = (r, g, b, a)
            pixels.append(color)
        image.putdata(pixels)
        path = output / f"material-{material}.png"
        image.save(path)
        yield material, path
        mesh = costume.u32(mesh + 4)
        material += 1
