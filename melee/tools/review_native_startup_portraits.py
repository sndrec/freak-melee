"""Check real offscreen RGBA portraits for every starter in one hidden/muted run."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import time
from PIL import Image, ImageDraw
from review_native_css_room import Client


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'iso', 'output']:
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    empty = output / 'empty'; empty.mkdir()
    portraits = output / 'portraits'; portraits.mkdir()
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = 'http://localhost:1'
    cache = output / 'gpu-cache'; cache.mkdir()
    os.environ['FREAK_MELEE_RENDER_CACHE'] = str(cache)
    client = None
    checks, failures, screenshots = [], [], []
    try:
        client = Client(args.exe.resolve(), args.iso.resolve(), empty, output / 'client', 'Portrait review',
            window_size=(1280, 960), chat_endpoint='http://localhost:1', write_profile=False)
        client.click('setup-next'); client.enter('setup-name', 'Portrait review')
        client.click('setup-next'); client.click('setup-next'); client.click('setup-skip-controller')
        for index in range(24):
            client.element('create-clone'); time.sleep(.2)
            state = client.inspect('startup-roster')
            match = re.search(r'selected=(\d+) kind=(\d+) name=(.*)', state)
            if not match or int(match[1]) != index: raise RuntimeError(state)
            if 'accepted=1' not in client.inspect('startup-portrait-save'):
                failures.append(f'{match[3]}: portrait request was rejected')
            else:
                for _ in range(100):
                    status = client.inspect('startup-portrait')
                    if re.search(r'state=[34]\b', status): break
                    time.sleep(.1)
                if not re.search(r'state=3\b', status):
                    failures.append(f'{match[3]}: export did not finish: {status}')
                else:
                    path = portraits / f'{index:02}-{match[2]}.png'
                    shutil.copyfile(client.data / 'content/starter-portrait.png', path)
                    with Image.open(path) as image:
                        if image.mode != 'RGBA' or image.size != (256, 256):
                            failures.append(f'{match[3]}: wrong image layout')
                        else:
                            alpha = image.getchannel('A'); box = alpha.getbbox()
                            if not box or alpha.getextrema() != (0, 255):
                                failures.append(f'{match[3]}: empty or opaque backdrop')
                            elif box[0] == 0 or box[1] == 0 or box[2] == 256 or box[3] == 256:
                                failures.append(f'{match[3]}: geometry touches portrait edge: {box}')
                            checks.append({'name': match[3], 'file': str(path), 'bounds': box})
                    if match[3] == 'Ice Climbers' and 'subjects=2' not in status:
                        failures.append('Ice Climbers did not include both models')
            if index in [0, 6, 12, 21]: screenshots.append(str(client.screenshot()))
            client.click('setup-fighter-next')
        sheet = Image.new('RGB', (6 * 160, 4 * 180), '#39343e')
        labels = ImageDraw.Draw(sheet)
        for i, check in enumerate(checks):
            with Image.open(check['file']) as portrait:
                portrait = portrait.resize((160, 160))
                sheet.paste(portrait, ((i % 6) * 160, (i // 6) * 180), portrait)
            labels.text(((i % 6) * 160 + 3, (i // 6) * 180 + 162), check['name'], fill='white')
        sheet.save(output / 'overview.png')
        (output / 'receipt.json').write_text(json.dumps({'portraits': checks, 'failures': failures,
            'screenshots': screenshots, 'exe_sha256': hashlib.sha256(args.exe.read_bytes()).hexdigest(),
            'cold_pipeline_cache': str(cache),
            'limitations': ['Developer capture path; the user-facing portrait and CSS placement steps are not connected yet.']}, indent=2) + '\n')
        if failures: raise RuntimeError('\n'.join(failures))
    finally:
        if client: client.stop()
    print('24 RGBA portrait captures passed, including paired Ice Climbers')


if __name__ == '__main__':
    main()
