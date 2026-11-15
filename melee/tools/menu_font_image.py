"""Draw the prepared native bitmap faces in offline artwork and film tools."""
from pathlib import Path
import struct
import unicodedata

from PIL import Image, ImageFilter


class FontAtlas:
    def __init__(self, path):
        raw = Path(path).read_bytes()
        width,height,self.cap,count,fmt,pairs = struct.unpack_from('<6H',raw)
        if not self.cap or width != 1024 or not 0 < height <= 1024 or fmt not in (0,2):
            raise ValueError('Invalid native bitmap font '+str(path))
        records = list(struct.iter_unpack('<I5H2hH',raw[12:12+count*20]))
        start = 12+count*20+pairs*12
        pixels = raw[start:]
        if len(pixels) != width*height//(2 if fmt==0 else 1):
            raise ValueError('Truncated native bitmap font '+str(path))
        sheet = Image.new('L',(width,height)); offset=0
        rows=8 if fmt==0 else 4
        for by in range(0,height,rows):
            for bx in range(0,width,8):
                for y in range(rows):
                    for x in range(8):
                        i=y*8+x;value=pixels[offset+(i//2 if fmt==0 else i)]
                        if fmt==0:value>>=4 if i%2==0 else 0
                        # The low IA4 nibble is the white fill; create outlines
                        # at the requested output size instead of scaling theirs.
                        sheet.putpixel((bx+x,by+y),(value&15)*17)
                offset+=32
        self.glyphs={code:(sheet.crop((x,y,x+w,y+h)),advance,left,top)
                     for code,x,y,w,h,advance,left,top,_ in records}

    def width(self, text, height):
        return sum(self.glyphs.get(ord(c),self.glyphs[0xfffd])[1]
                   for c in unicodedata.normalize('NFC',text))*height/self.cap

    def draw(self, image, text, x, y, height, color='white', centered=False,
             outline=0, outline_color='black'):
        from PIL import ImageColor
        if not text:return
        scale=height/self.cap
        glyphs=[self.glyphs.get(ord(c),self.glyphs[0xfffd]) for c in unicodedata.normalize('NFC',text)]
        left=min(0,min(g[2] for g in glyphs));top=min(0,min(g[3] for g in glyphs))
        width=sum(g[1] for g in glyphs)
        bottom=max(g[3]+g[0].height for g in glyphs)
        mask=Image.new('L',(width-left+8,bottom-top+8));cursor=-left
        for glyph,advance,gx,gy in glyphs:
            mask.paste(glyph,(cursor+gx,gy-top));cursor+=advance
        mask=mask.resize((max(1,round(mask.width*scale)),max(1,round(mask.height*scale))),Image.Resampling.LANCZOS)
        if centered:x-=width*scale/2;y-=height/2
        x+=left*scale;y+=top*scale
        pad=outline
        if outline:
            padded=Image.new('L',(mask.width+pad*2,mask.height+pad*2));padded.paste(mask,(pad,pad))
            border=padded.filter(ImageFilter.MaxFilter(outline*2+1))
            tint=ImageColor.getcolor(outline_color,'RGBA') if isinstance(outline_color,str) else outline_color
            layer=Image.new('RGBA',border.size,tint);layer.putalpha(border)
            position=(round(x)-pad,round(y)-pad)
            if image.mode=='RGBA': image.alpha_composite(layer,position)
            else: image.paste(layer,position,layer)
        tint=ImageColor.getcolor(color,'RGBA') if isinstance(color,str) else color
        layer=Image.new('RGBA',mask.size,tint);layer.putalpha(mask)
        position=(round(x),round(y))
        if image.mode=='RGBA': image.alpha_composite(layer,position)
        else: image.paste(layer,position,layer)
