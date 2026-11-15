# Build reproducible, context-specific GX glyph atlases. Source textures stay on
# the user's disc; the supplied CSS outline sheet is project-authored artwork.
import argparse, math, struct
from pathlib import Path
from PIL import Image, ImageDraw, ImageChops, ImageFilter
from prepare_menu_lettering import option_glyphs, caption_glyphs
from prepare_menu_unicode import expand
import re, xml.etree.ElementTree as ET
from prepare_disc_fonts import text_glyphs

CAPS = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'

def disc_file(iso, name):
    with iso.open('rb') as f:
        f.seek(0x424); offset, size = struct.unpack('>II', f.read(8))
        f.seek(offset); fst=f.read(size)
        count=struct.unpack_from('>I',fst,8)[0];strings=fst[count*12:]
        for i in range(1,count):
            flag,offset,size=struct.unpack_from('>III',fst,i*12)
            if flag>>24:continue
            pos=flag&0xffffff;end=strings.index(b'\0',pos)
            if strings[pos:end].decode()==name:
                f.seek(offset);return f.read(size)
    raise ValueError('Missing disc file '+name)

def mask(dat, descriptor):
    pixels,w,h,fmt=struct.unpack_from('>IHHI',dat,descriptor+32)
    if fmt!=0:raise ValueError('Expected original I4 lettering')
    raw=dat[32+pixels:];out=Image.new('L',(w,h));p=out.load();offset=0
    for by in range(0,h,8):
        for bx in range(0,w,8):
            for y in range(8):
                for x in range(8):
                    i=y*8+x;v=(raw[offset+i//2]>>(4 if i%2==0 else 0)&15)*17
                    if bx+x<w and by+y<h:p[bx+x,by+y]=v
            offset+=32
    return out

def collect(image, letters, glyphs):
    bbox=image.getbbox()
    if not bbox:return
    occupied=[x for x in range(image.width) if image.crop((x,0,x+1,image.height)).getbbox()]
    runs=[]
    for x in occupied:
        if not runs or x!=runs[-1][-1]+1:runs.append([x])
        else:runs[-1].append(x)
    if len(runs)!=len(letters):return
    for c,run in zip(letters,runs):
        glyphs.setdefault(c,image.crop((run[0],bbox[1],run[-1]+1,bbox[3])))

def complete_caps(glyphs):
    # Missing Q/V are constructed from this face's O/A, not a substitute font.
    if 'Q' not in glyphs:
        o=glyphs['O'];q=Image.new('L',(o.width+2,o.height+2));q.paste(o,(0,0))
        d=ImageDraw.Draw(q);d.line((o.width*.55,o.height*.65,o.width+1,o.height+1),fill=255,width=max(1,o.width//7));glyphs['Q']=q
    if 'V' not in glyphs:
        a=glyphs['A'];v=Image.new('L',a.size);d=ImageDraw.Draw(v)
        d.line((1,0,a.width//2,a.height-1,a.width-2,0),fill=255,width=max(1,a.width//6));glyphs['V']=v
    if 'J' not in glyphs:
        i=glyphs['I'];o=glyphs['O'];j=Image.new('L',(max(i.width*2,o.width//2),o.height));d=ImageDraw.Draw(j)
        thick=max(1,i.width//2);d.line((j.width-2,0,j.width-2,j.height-thick-1,j.width//2,j.height-1,1,j.height-3),fill=255,width=thick);glyphs['J']=j
    missing=set(CAPS)-glyphs.keys()
    if missing:raise ValueError('Unaccounted original glyphs '+str(missing))

def angular_glyphs(path, cap_height=24):
    """Rasterize the provided angular outline glyphs with their shared baseline.

    Each glyph has a Unicode ID and absolute M/L/Z contours. Sheet placement
    transforms are irrelevant: paths use local glyph coordinates. Even-odd
    filling preserves counters and disconnected punctuation marks.
    """
    glyphs={};scale=cap_height/80;supersample=8
    for group in ET.parse(path).getroot().findall('{*}g'):
        code=group.get('id','')
        if not re.fullmatch(r'glyph-[0-9A-Fa-f]{4,6}',code):continue
        contours=[]
        for shape in group.findall('{*}path'):
            tokens=re.findall(r'[A-Za-z]|[-+]?(?:\d*\.\d+|\d+)',shape.get('d',''))
            i=0;points=[]
            while i<len(tokens):
                command=tokens[i];i+=1
                if command in ('M','L'):
                    if i+1>=len(tokens):raise ValueError('Incomplete angular glyph '+code)
                    if command=='M' and points:raise ValueError('Unclosed angular contour '+code)
                    points.append((float(tokens[i]),float(tokens[i+1])));i+=2
                elif command=='Z':
                    if len(points)<3:raise ValueError('Invalid angular contour '+code)
                    contours.append(points);points=[]
                else:raise ValueError('Unsupported angular outline command '+command)
            if points:raise ValueError('Unclosed angular contour '+code)
        if not contours:raise ValueError('Missing angular outline '+code)
        left=min(x for contour in contours for x,y in contour)
        right=max(x for contour in contours for x,y in contour)
        bottom=max(80,max(y for contour in contours for x,y in contour))
        size=(max(1,math.ceil((right-left)*scale)),math.ceil(bottom*scale))
        high=tuple(n*supersample for n in size);fill=Image.new('1',high)
        for contour in contours:
            layer=Image.new('1',high)
            ImageDraw.Draw(layer).polygon([((x-left)*scale*supersample,y*scale*supersample) for x,y in contour],fill=1)
            fill=ImageChops.logical_xor(fill,layer)
        glyphs[chr(int(code[6:],16))]=fill.convert('L').resize(size,Image.Resampling.LANCZOS)
    # CSS tile names use capitals. The supplied set provides every printable
    # ASCII symbol except backtick, for which the apostrophe shape is used.
    glyphs[' ']=Image.new('L',(10,cap_height))
    glyphs['`']=glyphs["'"]
    missing={chr(c) for c in range(32,127) if not chr(c).islower()}-glyphs.keys()
    if missing:raise ValueError('Missing angular glyphs '+str(missing))
    return glyphs

def atlas(path, glyphs, cap_height, tracking, caps=False, outline=False):
    glyphs, compositions = expand(glyphs, cap_height, tracking, caps)
    if len(glyphs)>512 or len(compositions)>256:raise ValueError('GX font metadata capacity')
    sheet=Image.new('LA' if outline else 'L',(1024,1024));metrics=[]
    x=y=1;row_height=0
    for c,(g,left,top,advance) in sorted(glyphs.items()):
        if outline:
            fill=Image.new('L',(g.width+4,g.height+4));fill.paste(g,(2,2))
            g=Image.merge('LA',(fill,fill.filter(ImageFilter.MaxFilter(5))))
            left-=2;top-=2
        if g.width>128 or g.height>96 or advance>160:raise ValueError('Oversized glyph '+ascii(c))
        if x+g.width+1>1024:
            x=1;y+=row_height+2;row_height=0
        if y+g.height+1>1024:raise ValueError('GX font atlas capacity')
        sheet.paste(g,(x,y));metrics.append((ord(c),x,y,g.width,g.height,advance,left,top,0))
        x+=g.width+2;row_height=max(row_height,g.height)
    sheet=sheet.crop((0,0,1024,(y+row_height+8)//8*8))
    raw=bytearray()
    for by in range(0,sheet.height,4 if outline else 8):
        for bx in range(0,sheet.width,8):
            for yy in range(4 if outline else 8):
                if outline:
                    for xx in range(8):
                        intensity,alpha=sheet.getpixel((bx+xx,by+yy))
                        raw.append((alpha>>4)<<4 | (intensity>>4))
                else:
                    for xx in range(0,8,2):
                        raw.append((sheet.getpixel((bx+xx,by+yy))>>4)<<4 | sheet.getpixel((bx+xx+1,by+yy))>>4)
    data=(struct.pack('<6H',sheet.width,sheet.height,cap_height,len(metrics),2 if outline else 0,len(compositions))+
          b''.join(struct.pack('<I5H2hH',*m)for m in metrics)+
          b''.join(struct.pack('<3I',*pair)for pair in compositions)+raw)
    path.parent.mkdir(parents=True,exist_ok=True)
    if not path.exists() or path.read_bytes()!=data:path.write_bytes(data)
    sheet.save(path.with_suffix('.png'))

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--iso',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();root=Path(__file__).resolve().parents[1]
    # Extracting the HSD face also validates the original executable's hash.
    fallback=text_glyphs(args.iso)
    data=disc_file(args.iso,'GmRst.usd')
    name_seeds=[(0x29d80,'KIRBY'),(0x29d98,'BOWSER'),(0x29dc8,'LUIGI'),(0x29de0,'MARIO'),(0x29df8,'MARTH'),(0x29e28,'NESS'),(0x29e40,'PEACH'),(0x29ed0,'ZELDA'),(0x29ee8,'FALCO'),(0x2a05c,'FOX')]
    winner_seeds=[(0x2ab5c,'KIRBY'),(0x2ab74,'BOWSER'),(0x2abbc,'MARIO'),(0x2abd4,'MARTH'),(0x2ac04,'NESS'),(0x2ac1c,'PEACH'),(0x2acac,'ZELDA'),(0x2ab2c,'FOX'),(0x2ac7c,'SAMUS'),(0x2aba4,'LUIGI'),(0x2ab14,'DK')]
    menu=disc_file(args.iso,'MnMaAll.usd')
    glyphs,height=option_glyphs(menu,fallback)
    atlas(args.output/'menu-option.bin',glyphs,height,0,outline=True)
    glyphs,height=caption_glyphs(menu,fallback)
    atlas(args.output/'menu-caption.bin',glyphs,height,1)
    for face,seeds,height,tracking in [('result-name',name_seeds,15,4),('result-winner',winner_seeds,28,5)]:
        glyphs={}
        for offset,letters in seeds:collect(mask(data,offset),letters,glyphs)
        if face=='result-winner':glyphs['Z']=mask(data,0x2acac).crop((42,0,76,28))
        complete_caps(glyphs)
        for c,im in fallback.items():
            if c not in glyphs:
                h=height if c.isalnum() else min(height,im.height)
                glyphs[c]=im.resize((max(1,round(im.width*h/im.height)),h),Image.Resampling.LANCZOS)
        atlas(args.output/(face+'.bin'),glyphs,height,tracking,True)
    # HSD is the original game's proportional body-text face. The CSS banner
    # uses its capitals; menu ribbons, italic captions and results keep theirs.
    atlas(args.output/'menu-heading.bin',fallback,24,1,True)
    atlas(args.output/'notice.bin',fallback,24,1)
    glyphs=angular_glyphs(root/'assets/ui/fonts/angular_glyph_set.svg')
    atlas(args.output/'css-tile.bin',glyphs,24,3,True)
