"""Record directed native gameplay plates for the captioned startup movie.

Actors use installed source fighters, normal moves and actual costume packages.
Direction cues only place actors and cameras; the native simulation supplies
motion, collisions, particles and sound. No visible window or speaker playback.
"""
import argparse
import json
import os
from pathlib import Path
import re
import time

from capture_native_video import NativeVideo
from review_native_css_room import Client


def common_motions():
    header = Path(__file__).resolve().parents[1] / 'src/melee/ft/kinds/ftCommon/forward.h'
    body = header.read_text().split('typedef enum ftCommon_MotionState {', 1)[1].split('}', 1)[0]
    value = -2
    result = {}
    for name, explicit in re.findall(r'ftCo_MS_(\w+)(?:\s*=\s*(-?\d+))?\s*,', body):
        value = int(explicit) if explicit else value + 1
        result[name] = value
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'iso', 'profile', 'cast', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--shots', nargs='+', default=['welcome', 'tilt', 'aerial', 'fireball', 'overpowered', 'neutral', 'outfits'])
    parser.add_argument('--outfits', nargs='+', default=['nook_island', 'nook_sweater'])
    args = parser.parse_args()
    if not 1 <= len(args.outfits) <= 3: raise ValueError('Choose one to three outfits beside the original Falco.')
    cast = json.loads(args.cast.read_text())
    root = args.output.resolve()
    root.mkdir(parents=True, exist_ok=False)
    cache = root/'render-cache'; cache.mkdir()
    os.environ['FREAK_MELEE_RENDER_CACHE'] = str(cache)
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = 'http://127.0.0.1:1'
    motions = common_motions()
    receipt = {'shots': {}, 'cast': cast}
    client = None
    started = 0

    def cue(command):
        client.command('studio', command)
        events.append({'second': round(time.monotonic() - started, 3), 'studio': command})

    def pose(port, motion, x, y=0, facing=1, frame=0):
        cue(f'pose,{port},{motions[motion]},{frame},{x},{y},{facing}')

    for shot in args.shots:
        events = []
        key = 'Mario' if shot == 'welcome' else 'Ganondorf' if shot in ('overpowered', 'neutral') else 'Falco' if shot == 'outfits' else 'MarioRemix'
        extra = []
        if shot == 'outfits':
            for port, outfit in enumerate(args.outfits, 2):
                extra += [f'--p{port}', 'human:custom', f'--p{port}-fighter', cast['fighters']['Falco']['id'], f'--p{port}-skin', cast['costumes'][outfit]['id']]
            for port in range(len(args.outfits)+2,5): extra += [f'--p{port}', 'off']
        else:
            extra = ['--p2', 'human:jigglypuff' if shot == 'neutral' else 'human:luigi', '--p3', 'off', '--p4', 'off']
        try:
            client = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(), root / shot, 'Movie studio',
                            window_size=(1280, 960), write_profile=False,
                            launch_args=('--match', '--stage', 'final-destination', '--stocks', '1', '--items', 'off',
                                         '--content-root', cast['content'], '--p1', 'human:custom', '--p1-fighter', cast['fighters'][key]['id'], *extra))
            for port in range(4): client.command('hold', port, 'sx=0')
            time.sleep(4)
            started = time.monotonic()
            cue('music,off'); cue('hud,0'); cue('appearance,1')
            cue('camera,18,18,48,0,8,0,38')
            pose(0, 'Wait', 0)
            pose(1, 'Wait', 65, facing=-1)
            if shot == 'outfits':
                distance = 90 if len(args.outfits)==3 else 70
                cue(f'camera,0,18,{distance},0,8,0,38')
                for port in range(len(args.outfits)+1): pose(port, 'Wait', 65)
            if shot in ('tilt', 'overpowered'):
                pose(0, 'Wait', -9)
                pose(1, 'Wait', 4, facing=-1)
                cue('camera,5,15,55,0,8,0,38')
            if shot == 'neutral':
                pose(0, 'Wait', -7); pose(1, 'Wait', 7, facing=-1)
                cue('camera,-5,18,65,0,10,0,38')
            time.sleep(.6)
            client.screenshot()
            started = time.monotonic()
            with NativeVideo(client, root / (shot + '.mp4')):
                time.sleep(1)
                if shot == 'welcome':
                    for step in range(30):
                        cue(f'camera,{18-step*1.2:.2f},18,48,0,8,0,38')
                        time.sleep(.1)
                    pose(0, 'AppealSR', 0)
                    time.sleep(3)
                elif shot == 'tilt':
                    client.command('tap', 0, 'a', 'sx=30', 'frames=2')
                    time.sleep(2)
                    client.command('studio', 'state')
                    pose(0, 'Wait', -9); pose(1, 'Wait', 4, facing=-1)
                    time.sleep(.6)
                    client.command('tap', 0, 'a', 'sx=30', 'frames=2')
                    time.sleep(2)
                elif shot == 'aerial':
                    cue('camera,12,22,65,0,14,0,38')
                    cue('fall,0,-6,27');time.sleep(.08)
                    client.command('tap', 0, 'a', 'sx=30', 'frames=2')
                    time.sleep(2.3)
                    cue('fall,0,-6,32');time.sleep(.08)
                    client.command('tap', 0, 'a', 'sx=30', 'frames=2')
                    time.sleep(2.6)
                elif shot == 'fireball':
                    cue('camera,3,18,75,5,9,0,38')
                    pose(0, 'Wait', -22)
                    for _ in range(3):
                        client.command('tap', 0, 'b', 'frames=2');time.sleep(1.2)
                    time.sleep(1.5)
                elif shot == 'overpowered':
                    client.command('tap', 0, 'a', 'frames=2')
                    time.sleep(3)
                elif shot == 'neutral':
                    pose(0, 'AppealSR', -7)
                    time.sleep(1.4)
                    cue('particle,0,202,2.5')
                    time.sleep(.08)
                    cue('appearance,0')
                    time.sleep(.5)
                    client.command('tap', 1, 'b', 'sy=80', 'frames=2')
                    time.sleep(2.2)
                    client.screenshot();cue('state');time.sleep(1.5)
                elif shot == 'outfits':
                    positions = (-27,-9,9,27) if len(args.outfits)==3 else (-20,0,20) if len(args.outfits)==2 else (-12,12)
                    for port, x in enumerate(positions):
                        cue(f'fall,{port},{x},43')
                        time.sleep(.9)
                        pose(port, 'AppealSR', x)
                        time.sleep(.35)
                    for port, x in enumerate(positions): pose(port, 'AppealSR', x)
                    time.sleep(2.8)
                time.sleep(.6)
                client.screenshot()
            cue('state')
            receipt['shots'][shot] = {'file': str(root / (shot + '.mp4')), 'events': events,
                                      'seconds': round(time.monotonic()-started, 3)}
            (root / 'takes.json').write_text(json.dumps(receipt, indent=2))
            print(f'Recorded {shot}', flush=True)
        finally:
            if client: client.stop(); client = None


if __name__ == '__main__':
    main()
