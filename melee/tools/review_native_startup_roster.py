"""Review every starter preview in one isolated, hidden and muted native run."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import time
from review_native_css_room import Client


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'iso', 'output']:
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    empty = output / 'empty'; empty.mkdir()
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = 'http://localhost:1'
    client = None
    states, screenshots, failures = [], [], []
    try:
        client = Client(args.exe.resolve(), args.iso.resolve(), empty, output / 'client', 'Roster review',
            window_size=(1280, 960), chat_endpoint='http://localhost:1', write_profile=False)
        client.click('setup-next'); client.enter('setup-name', 'Roster review')
        client.click('setup-next'); client.click('setup-next'); client.click('setup-skip-controller')
        for index in range(24):
            client.element('create-clone'); time.sleep(.3)
            state = client.inspect('startup-roster')
            states.append(state)
            selected = re.search(r'startup-roster count=24 selected=(\d+) kind=(\d+) name=(.*)', state)
            if not selected or int(selected[1]) != index:
                failures.append(f'Wrong carousel position {index}: {state}')
            else:
                kind = int(selected[2])
                if not re.search(rf'menu-fighter slot=[012] kind={kind} ', state):
                    failures.append(f'Missing main model for {selected[3]}')
                screenshots.append(str(client.screenshot()))
                if selected[3] == 'Ice Climbers' and not re.search(r'menu-fighter slot=[345] kind=11 ', state):
                    failures.append('Ice Climbers preview is missing Nana')
            client.click('setup-fighter-next')
        assert 'selected=0 ' in client.inspect('startup-roster')
        client.click('setup-back'); assert 'active=1 step=3' in client.inspect('startup')
        unloaded = client.inspect('startup-roster')
        if 'menu-fighter slot=' in unloaded: failures.append('Leaving starter selection retained preview models')
        client.click('setup-skip-controller'); client.element('create-clone')
        if not re.search(r'menu-fighter slot=[012] kind=0 ', client.inspect('startup-roster')):
            failures.append('Reopening starter selection did not reload Mario')
        from PIL import Image, ImageDraw
        overview = Image.new('RGB', (1280, 6 * 258), '#171318')
        draw = ImageDraw.Draw(overview)
        for i, filename in enumerate(screenshots):
            with Image.open(filename) as frame:
                overview.paste(frame.convert('RGB').resize((320, 240)), ((i % 4) * 320, (i // 4) * 258))
            draw.text(((i % 4) * 320 + 4, (i // 4) * 258 + 242), f'{i + 1}', fill='white')
        overview.save(output / 'overview.png')
        (output / 'receipt.json').write_text(json.dumps({
            'states': states, 'screenshots': screenshots, 'failures': failures,
            'checks': ['24 starter previews load in order without Kirby', 'Carousel wraps and releases its model owners on Back',
                'Reopening reloads the selected preview'],
            'exe_sha256': hashlib.sha256(args.exe.read_bytes()).hexdigest(),
            'limitations': ['This pass checks and captures the carousel; portrait capture and direct CSS placement remain open.'],
        }, indent=2) + '\n')
        if failures: raise RuntimeError('\n'.join(failures))
    finally:
        if client: client.stop()
    print('24 starter previews and carousel lifetime checks passed')


if __name__ == '__main__':
    main()
