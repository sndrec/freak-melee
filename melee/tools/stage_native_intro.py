"""Render native pose studies and assemble the first intro storyboard.

The output is a storyboard animatic, with pending footage labelled. Stock
costumes establish blocking; no generated packages or disc artwork enter Git.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

from PIL import Image, ImageDraw, ImageFont
from review_native_css_room import Client


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'iso', 'profile', 'output']:
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--ffmpeg', default='ffmpeg')
    parser.add_argument('--music', type=Path)
    args = parser.parse_args()
    setup = args.profile / 'Freak Melee/content/startup.txt'
    if not setup.is_file() or setup.read_text().splitlines()[0].split()[2] != '1':
        raise ValueError('Choose a completed isolated startup profile for --profile.')
    source = Path(__file__).resolve().parents[1]
    recipe = source / 'assets/intro/storyboard.json'
    board = json.loads(recipe.read_text())
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    width, height = board['size']
    font = lambda size: ImageFont.truetype(str(source / 'assets/fonts/FolkPro-Medium.otf'), size)
    header, note_font = font(23), font(19)
    caption_font = ImageFont.truetype(str(source / 'assets/fonts/FolkPro-Heavy.otf'), 46)
    heavy = ImageFont.truetype(str(source / 'assets/fonts/FolkPro-Heavy.otf'), 65)
    empty = output / 'empty'; empty.mkdir()
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = 'http://localhost:1'
    client, images, captures = None, [], {}

    def text_lines(text, limit, face):
        lines, line = [], ''
        for word in text.split():
            attempt = (line + ' ' + word).strip()
            if line and face.getlength(attempt) > limit:
                lines.append(line); line = word
            else:
                line = attempt
        return lines + ([line] if line else [])

    def centered(draw, text, y, face, color=(250, 250, 248)):
        draw.text(((width-face.getlength(text))/2, y), text, font=face, fill=color,
                  stroke_width=2, stroke_fill=(12, 10, 15))

    try:
        client = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(),
                        output / 'native', 'Intro staging', window_size=(960, 720),
                        write_profile=False)
        client.element('nav-5')
        for index, shot in enumerate(board['shots']):
            image = Image.new('RGB', (width, height), (12, 12, 20))
            draw = ImageDraw.Draw(image)
            # Quiet stage for composition. These are storyboard marks, not a game stage.
            draw.ellipse((110, 440, 850, 615), fill=(25, 25, 39))
            for actor in shot['actors']:
                key = (actor['kind'], actor.get('costume', 0), actor['animation'], actor['frame'], actor['yaw'])
                if key not in captures:
                    number = len(captures)
                    specification = ','.join(map(str, [number, *key[:4], key[4], .08, .8, 0, 0]))
                    result = client.inspect('menu-shot:' + specification)
                    if 'menu-shot accepted=1' not in result:
                        raise RuntimeError(f"Could not stage {shot['id']}: {result}")
                    path = client.folder / f'menu-shot-{number:05}.png'
                    deadline = time.monotonic() + 90
                    while time.monotonic() < deadline:
                        if path.exists() and path.read_bytes()[-8:-4] == b'IEND': break
                        time.sleep(.1)
                    else:
                        raise RuntimeError(f'Native export did not complete: {path}')
                    captures[key] = path
                with Image.open(captures[key]) as rendered:
                    bounds = rendered.getbbox()
                    if not bounds: raise RuntimeError(f'Empty native actor: {key}')
                    actor_image = rendered.crop(bounds)
                    scale = actor['size'] / actor_image.height
                    actor_image = actor_image.resize((round(actor_image.width*scale), actor['size']), Image.Resampling.LANCZOS)
                    image.paste(actor_image, (round(actor['x']-actor_image.width/2), actor['feet']-actor_image.height), actor_image)
            draw = ImageDraw.Draw(image)
            if shot.get('censor'):
                draw.rectangle((350, 340, 610, 470), fill=(0, 0, 0))
                centered(draw, 'CENSORED', 375, header)
            if shot.get('wireframe'):
                # Clearly a staging marker; the real filter must supply the final actor.
                draw.ellipse((650, 190, 715, 255), outline=(215, 205, 235), width=3)
                for a, b in [((680,255),(680,405)),((680,290),(615,360)),((680,290),(745,360)),((680,405),(625,555)),((680,405),(735,555))]:
                    draw.line((a,b), fill=(215,205,235), width=3)
                draw.text((600,570), 'FILTER ACTOR: TO STAGE', font=note_font, fill=(183,175,195))
            if shot['id'] == 'end':
                centered(draw, 'FREAK MELEE', 275, heavy, (238,203,88))
            draw.rectangle((0,0,width,65), fill=(20,17,28))
            draw.text((28,18), f"{index+1:02}  /  {shot['id'].upper().replace('-', ' ')}", font=header, fill=(232,202,105))
            draw.text((width-228,22), 'STORYBOARD / NATIVE POSES', font=font(12), fill=(178,166,190))
            for line_number, line in enumerate(text_lines(shot['caption'], width-110, caption_font)):
                centered(draw, line, shot.get('caption_y',90)+line_number*54, caption_font)
            if shot.get('pending'):
                draw.text((28,670), 'FOOTAGE TO FILM', font=note_font, fill=(189,162,133))
            path = output / f'{index+1:02}-{shot["id"]}.png'
            image.save(path); images.append((path, shot))
    finally:
        if client: client.stop()

    contact = Image.new('RGB', (960, 3*300), (12,12,20))
    draw = ImageDraw.Draw(contact)
    for i, (path, shot) in enumerate(images):
        with Image.open(path) as panel:
            contact.paste(panel.resize((320,240), Image.Resampling.LANCZOS), ((i%3)*320, (i//3)*300))
        for line_index, line in enumerate(text_lines(shot['direction'], 300, font(12))[:3]):
            draw.text(((i%3)*320+10,(i//3)*300+245+line_index*15),line,font=font(12),fill=(205,196,213))
    contact.save(output / 'storyboard.png')
    concat = output / 'boards.ffconcat'
    with concat.open('w', encoding='utf-8') as file:
        file.write('ffconcat version 1.0\n')
        for path, shot in images:
            file.write(f"file '{path.name}'\nduration {shot['seconds']}\n")
        file.write(f"file '{images[-1][0].name}'\n")
    command = [args.ffmpeg, '-hide_banner', '-loglevel', 'error', '-y', '-safe', '0', '-i', str(concat)]
    duration = sum(shot['seconds'] for _, shot in images)
    if args.music:
        command += ['-stream_loop', '-1', '-i', str(args.music.resolve()), '-af', f'afade=t=out:st={duration-3}:d=3', '-c:a', 'aac', '-b:a', '128k']
    command += ['-t', str(duration), '-r', str(board['fps']), '-c:v', 'libx264', '-crf', '22', '-pix_fmt', 'yuv420p', '-movflags', '+faststart', str(output / 'intro-storyboard.mp4')]
    subprocess.run(command, check=True, creationflags=subprocess.CREATE_NO_WINDOW)
    (output / 'receipt.json').write_text(json.dumps({
        'recipe': str(recipe), 'recipe_sha256': hashlib.sha256(recipe.read_bytes()).hexdigest(),
        'exe_sha256': hashlib.sha256(args.exe.read_bytes()).hexdigest(),
        'native_poses': [dict(kind=k[0], costume=k[1], animation=k[2], frame=k[3], yaw=k[4], png=str(p)) for k,p in captures.items()],
        'seconds': duration, 'status': 'Storyboard animatic. Gameplay, custom outfits and appearance-filter footage remain to film.'
    }, indent=2)+'\n')
    print(f'{len(images)} boards; {len(captures)} native pose captures; {duration}s storyboard animatic')


if __name__ == '__main__':
    main()
