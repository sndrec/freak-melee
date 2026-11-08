"""Cut native footage into the captioned startup film.

The edit recipe contains source trims, framing and timing. Caption text always
comes from storyboard.json. Generated media and disc-derived plates stay outside
Git; the recipe and this tool preserve an editable cut.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

from PIL import Image, ImageDraw, ImageFont


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--plates', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--recipe', type=Path)
    parser.add_argument('--ffmpeg', default='ffmpeg')
    parser.add_argument('--only', nargs='+', type=int, help='Rebuild these numbered segments before assembling.')
    args = parser.parse_args()
    source = Path(__file__).resolve().parents[1]
    recipe_path = args.recipe or source/'assets/intro/edit.json'
    recipe = json.loads(recipe_path.read_text())
    captions = [c['text'] for s in json.loads((source/'assets/intro/storyboard.json').read_text())['shots'] for c in s['captions']]
    root = args.output.resolve();root.mkdir(parents=True, exist_ok=True)
    plates = args.plates.resolve()
    width, height, fps = 960, 720, 60
    heavy = source/'assets/fonts/FolkPro-Heavy.otf'
    medium = source/'assets/fonts/FolkPro-Medium.otf'

    def run(command, label):
        with (root/(label+'.log')).open('w') as log:
            subprocess.run([args.ffmpeg, '-hide_banner', '-loglevel', 'warning', '-y', *command], stdout=log, stderr=log, check=True)

    def lines(text, font, limit):
        rows, row = [], ''
        for word in text.split():
            attempt = (row+' '+word).strip()
            if row and font.getlength(attempt)>limit:
                rows.append(row);row=word
            else:row=attempt
        return rows+([row] if row else [])

    def caption_image(number, segment):
        image = Image.new('RGBA', (width, height))
        draw = ImageDraw.Draw(image)
        if segment.get('censor'):
            draw.rectangle((215, 255, 545, 510), fill=(0,0,0,255))
            face = ImageFont.truetype(str(heavy), 42)
            draw.text((380,382), 'CENSORED', font=face, anchor='mm', fill='white')
        if segment.get('logo'):
            draw.rectangle((0,0,width,height), fill=(0,0,0,145))
            face = ImageFont.truetype(str(heavy), 98)
            draw.text((480,295), 'FREAK', font=face, anchor='mm', fill='white', stroke_width=3, stroke_fill='black')
            face = ImageFont.truetype(str(heavy), 88)
            draw.text((480,393), 'MELEE', font=face, anchor='mm', fill=(245,210,72), stroke_width=3, stroke_fill='black')
        if segment.get('credits'):
            draw.rectangle((0,0,width,height), fill=(0,0,0,225))
            y=176
            for text, size, color in [
                ('FREAK MELEE',54,'white'),
                ('Tom Nook Falco',32,(245,210,72)),
                ('LarsBars / @RealLarsBars',29,'white'),
                ('ssbmnucleus.net/post/17634',23,(200,198,211)),
                ('Melee assets / “How to Play” music',25,(245,210,72)),
                ('Nintendo / HAL Laboratory',27,'white'),
            ]:
                face=ImageFont.truetype(str(medium if size<40 else heavy),size)
                draw.text((480,y),text,font=face,anchor='mm',fill=color)
                y += 70 if size==54 else 49
        if 'caption' in segment:
            text = captions[segment['caption']-1]
            face = ImageFont.truetype(str(heavy),segment.get('font',46))
            rows = lines(text, face, 860)
            step = round(face.size*1.2)
            y = segment.get('caption_y',48)
            if segment.get('caption_scrim'):
                for row in range(220):
                    draw.line((0,row,width,row),fill=(0,0,0,round(185*(1-row/220))))
            for row in rows:
                draw.text(((width-face.getlength(row))/2,y), row, font=face,
                          fill=(255,255,250), stroke_width=3, stroke_fill=(9,8,15))
                y += step
        path = root/f'overlay-{number:02}.png';image.save(path)
        return path

    # Each caption can span several cuts, but the script must remain complete
    # and in order. This also catches an accidental omitted card in an edit.
    sequence=[]
    for segment in recipe['segments']:
        if 'caption' in segment and (not sequence or sequence[-1]!=segment['caption']):sequence.append(segment['caption'])
    if sequence!=list(range(1,len(captions)+1)):raise ValueError('The edit must include every supplied caption in order.')
    receipt={'caption_script':str(source/'assets/intro/storyboard.json'),'recipe_sha256':hashlib.sha256(recipe_path.read_bytes()).hexdigest(),'segments':[]}
    durations={}
    clock=0
    for i, segment in enumerate(recipe['segments']):
        speed=segment.get('speed',1)
        seconds=round(segment['seconds']*fps)/fps
        source_path=plates/segment['source']
        if not source_path.is_file():raise FileNotFoundError(source_path)
        if source_path not in durations:
            durations[source_path]=float(subprocess.check_output(['ffprobe','-v','error','-show_entries','format=duration','-of','csv=p=0',str(source_path)]))
        if segment.get('start',0)+seconds*speed>durations[source_path]+1/fps:
            raise ValueError(f'Segment {i} extends beyond its recorded footage.')
        output=root/f'segment-{i:02}.mkv'
        if args.only is None or i in args.only or not output.is_file():
            overlay=caption_image(i,segment)
            start=segment.get('start',0)
            crop=segment.get('crop',[18,0,1228,921])
            x,y,w,h=crop
            # A one-row encoder pad is excluded. Explicit crops permit film
            # close-ups of the real editor without changing its interface.
            video=f'[0:v]setpts=(PTS-STARTPTS)/{speed},crop={w}:{h}:{x}:{y},scale={width}:{height}:flags=lanczos:force_original_aspect_ratio=decrease,pad={width}:{height}:(ow-iw)/2:(oh-ih)/2,setsar=1,fps={fps},trim=duration={seconds}[plate];[plate][1:v]overlay=0:0:format=auto,format=yuv420p'
            if segment.get('fade_in'):video+=f",fade=t=in:st=0:d={segment['fade_in']}"
            if segment.get('fade_out'):video+=f",fade=t=out:st={seconds-segment['fade_out']}:d={segment['fade_out']}"
            video+='[v]'
            command=['-ss',str(start),'-i',str(source_path),'-i',str(overlay)]
            wav=source_path.with_suffix('.wav')
            sound=segment.get('sound',1)
            if wav.exists() and sound:
                command+=['-ss',str(start),'-i',str(wav)]
                audio=f'[2:a]asetpts=PTS-STARTPTS,atempo={speed},aresample=48000,volume={sound},apad,atrim=duration={seconds},afade=t=in:d=0.02,afade=t=out:st={max(0,seconds-.035)}:d=0.035[a]'
            else:
                command+=['-f','lavfi','-i','anullsrc=r=48000:cl=stereo']
                audio=f'[2:a]atrim=duration={seconds}[a]'
            run([*command,'-filter_complex',video+';'+audio,'-map','[v]','-map','[a]',
                 '-t',str(seconds),'-c:v','libx264','-preset','fast','-crf','18','-threads','4',
                 '-c:a','pcm_s16le','-f','matroska',str(output)],f'encode-{i:02}')
        receipt['segments'].append(dict(segment,index=i,timeline=round(clock,3),seconds=seconds,
            caption_text=captions[segment['caption']-1] if 'caption' in segment else None))
        clock+=seconds
        print(f'{i:02} {clock:6.2f}s {segment["source"]}',flush=True)
    concat=root/'segments.ffconcat'
    concat.write_text('ffconcat version 1.0\n'+''.join(f"file 'segment-{i:02}.mkv'\n" for i in range(len(recipe['segments']))))
    run(['-f','concat','-safe','0','-i',str(concat),'-c','copy',str(root/'picture-and-effects.mkv')],'concat')
    music=plates/recipe['music']
    mix=f'[1:a]aresample=48000,volume={recipe.get("music_gain",.75)},afade=t=in:d=0.4,afade=t=out:st={clock-4}:d=4[music];[0:a]volume=0.85[effects];[music][effects]amix=inputs=2:duration=longest:normalize=0,alimiter=limit=0.95:level=0,atrim=duration={clock}[a]'
    final=root/'freak-melee-intro-draft.mp4'
    run(['-i',str(root/'picture-and-effects.mkv'),'-ss',str(recipe.get('music_start',0)),'-i',str(music),
        '-filter_complex',mix,'-map','0:v','-map','[a]',
        '-vf',f'setpts=N/({fps}*TB)','-r',str(fps),'-vsync','cfr',
        '-c:v','libx264','-preset','medium','-crf','18','-threads','4','-c:a','aac','-b:a','192k',
        '-t',str(clock),'-movflags','+faststart',str(final)],'mix')
    receipt['seconds']=round(clock,3);receipt['movie']=str(final)
    (root/'edit-receipt.json').write_text(json.dumps(receipt,indent=2))
    (root/'captions.txt').write_text('\n\n'.join(captions),encoding='utf-8')
    print(final,flush=True)


if __name__=='__main__':main()
