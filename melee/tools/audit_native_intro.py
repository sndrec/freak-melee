"""Decode a finished intro and make one-second visual-review contact sheets."""
import argparse
import json
from pathlib import Path
import re
import subprocess

from PIL import Image, ImageDraw


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('movie',type=Path)
    args=parser.parse_args()
    movie=args.movie.resolve();root=movie.parent/'review';root.mkdir(exist_ok=True)
    info=json.loads(subprocess.check_output(['ffprobe','-v','error','-show_streams','-show_format','-of','json',str(movie)]))
    with (root/'decode.log').open('w') as log:
        subprocess.run(['ffmpeg','-hide_banner','-v','error','-i',str(movie),'-f','null','-'],stdout=log,stderr=log,check=True)
    with (root/'audio.log').open('w') as log:
        subprocess.run(['ffmpeg','-hide_banner','-i',str(movie),'-vn','-af','volumedetect','-f','null','-'],stdout=log,stderr=log,check=True)
    subprocess.run(['ffmpeg','-v','error','-y','-i',str(movie),'-vf','fps=1,scale=320:240','-q:v','3',str(root/'frame-%03d.jpg')],check=True)
    frames=sorted(root.glob('frame-*.jpg'))
    for page in range((len(frames)+15)//16):
        sheet=Image.new('RGB',(1280,1040),(17,17,23));draw=ImageDraw.Draw(sheet)
        for slot,path in enumerate(frames[page*16:(page+1)*16]):
            x,y=slot%4*320,slot//4*260
            with Image.open(path) as frame:sheet.paste(frame,(x,y))
            draw.text((x+7,y+242),f'{page*16+slot:02}s',fill='white')
        sheet.save(root/f'contact-{page+1:02}.jpg',quality=92)
    audio=(root/'audio.log').read_text()
    report={'duration':float(info['format']['duration']),'megabytes':movie.stat().st_size/1e6,
            'video':[{k:s.get(k) for k in ('codec_name','width','height','r_frame_rate','nb_frames')} for s in info['streams'] if s['codec_type']=='video'],
            'audio':[{k:s.get(k) for k in ('codec_name','sample_rate','channels')} for s in info['streams'] if s['codec_type']=='audio'],
            'levels':re.findall(r'(?:mean|max)_volume: [^\r\n]+',audio),
            'decode_errors':(root/'decode.log').read_text(),'contact_sheets':(len(frames)+15)//16}
    (root/'report.json').write_text(json.dumps(report,indent=2))
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
