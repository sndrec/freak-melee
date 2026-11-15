"""Complete punctuation with the recovered face's own strokes and letters."""
from PIL import Image, ImageDraw


def complete_symbols(glyphs, cap):
    glyphs = dict(glyphs)
    for char in 'abcdefghijklmnopqrstuvwxyz':
        glyphs.setdefault(char,glyphs[char.upper()])
    height = max(cap + 4, glyphs['H'].height)
    stroke = max(1, round(cap/12))

    def join(letters):
        pieces = [glyphs[c] for c in letters]
        image = Image.new('L', (sum(p.width for p in pieces), max(p.height for p in pieces)))
        x = 0
        for piece in pieces:
            image.paste(piece,(x,0)); x += piece.width
        return image

    for char, source in {'\u00ad':'-', '¦':'|', 'ŉ':"'n", '«':'<<', '»':'>>'}.items():
        glyphs.setdefault(char, join(source))
    for char, source in [('¡','!'),('¿','?')]:
        glyphs.setdefault(char,glyphs[source].transpose(Image.Transpose.ROTATE_180))
    glyphs.setdefault('¯',glyphs['_'].transpose(Image.Transpose.FLIP_TOP_BOTTOM))
    for char, base in [('¢','c'),('£','L'),('€','C'),('¥','Y')]:
        image = glyphs[base].copy(); draw = ImageDraw.Draw(image)
        w = image.width
        if char == '¢': draw.line((w/2,cap*.15,w/2,cap),fill=255,width=stroke)
        else:
            for y in ([cap*.5] if char=='£' else [cap*.4,cap*.6]):
                draw.line((1,y,w*.8,y),fill=255,width=stroke)
        glyphs.setdefault(char,image)
    for char, inner in [('©','C'),('®','R'),('¤',None),('°',None),('•',None)]:
        diameter = max(4,round(cap*(.25 if char=='•' else .35 if char=='°' else .9)))
        image = Image.new('L',(diameter+2,height)); draw = ImageDraw.Draw(image)
        y = cap*.5 if char=='•' else 0
        draw.ellipse((1,y,diameter,y+diameter-1),outline=255,width=stroke,
                     fill=255 if char=='•' else None)
        if inner:
            letter = glyphs[inner].resize((max(1,round(diameter*.6)),max(1,round(diameter*.6))),Image.Resampling.LANCZOS)
            image.paste(letter,((image.width-letter.width)//2,round(diameter*.2)))
        if char=='¤':
            for x,yy in [(1,0),(diameter,0),(1,diameter),(diameter,diameter)]:
                draw.line((x,yy,diameter/2,diameter/2),fill=255,width=stroke)
        glyphs.setdefault(char,image)
    for char, source in [('¨','..'),('´',"'"),('¸',',')]:
        image = join(source)
        if char in '¨´': image = image.transpose(Image.Transpose.FLIP_TOP_BOTTOM)
        glyphs.setdefault(char,image)
    glyphs.setdefault('¬',glyphs['-'].copy())
    draw = ImageDraw.Draw(glyphs['¬'])
    draw.line((glyphs['¬'].width-2,cap*.5,glyphs['¬'].width-2,cap*.7),fill=255,width=stroke)
    plus = glyphs['+']; minus = glyphs['-']
    image = Image.new('L',(max(plus.width,minus.width),height))
    image.paste(plus,(0,0)); image.paste(minus,(0,round(cap*.55)))
    glyphs.setdefault('±',image)
    image = glyphs['-'].copy(); draw = ImageDraw.Draw(image)
    for y in [cap*.2,cap*.8]:draw.ellipse((image.width/2-stroke,y-stroke,image.width/2+stroke,y+stroke),fill=255)
    glyphs.setdefault('÷',image)
    image = glyphs['u'].copy(); draw = ImageDraw.Draw(image)
    draw.line((stroke,cap*.5,stroke,height-1),fill=255,width=stroke)
    glyphs.setdefault('µ',image)
    image = join('P|'); glyphs.setdefault('¶',image)
    image = glyphs['S'].copy(); shifted = glyphs['S'].resize((image.width,max(1,round(image.height*.65))),Image.Resampling.LANCZOS)
    image.paste(shifted,(0,0)); image.paste(shifted,(0,round(cap*.45)))
    glyphs.setdefault('§',image)
    for char, a, b in [('¼','1','4'),('½','1','2'),('¾','3','4')]:
        upper = glyphs[a].resize((max(1,round(glyphs[a].width*.6)),max(1,round(height*.6))),Image.Resampling.LANCZOS)
        lower = glyphs[b].resize((max(1,round(glyphs[b].width*.6)),max(1,round(height*.6))),Image.Resampling.LANCZOS)
        image = Image.new('L',(upper.width+lower.width+stroke*2,height))
        image.paste(upper,(0,0));image.paste(lower,(upper.width+stroke*2,height-lower.height))
        ImageDraw.Draw(image).line((upper.width,cap,upper.width+stroke*2,cap*.2),fill=255,width=stroke)
        glyphs.setdefault(char,image)
    return glyphs
