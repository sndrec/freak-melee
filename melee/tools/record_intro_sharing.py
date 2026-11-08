"""Film an actual fighter gift between two isolated native clients.

Run a local signaling service first and supply its loopback endpoint. No global
chat or real account is used. The sender owns Mario Remix and Falco; the
recipient starts with Falco and receives Mario through the ordinary gift flow.
"""
import argparse
import json
import os
from pathlib import Path
import re
import secrets
import shutil
import struct
import time

from capture_native_video import NativeVideo
from review_native_css_room import Client


def layout(magic, identities):
    body = magic + struct.pack('<H', len(identities))
    for i, identity in enumerate(identities):
        body += struct.pack('<Q', 0) + bytes.fromhex(identity) + struct.pack('<4H', 345+i*110, 275, 92, 120)
    digest = 2166136261
    for byte in body: digest = ((digest ^ byte)*16777619) & 0xffffffff
    return body + struct.pack('<I', digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'iso', 'profile', 'cast', 'output'):
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--endpoint', required=True)
    args = parser.parse_args()
    if not args.endpoint.startswith(('http://127.0.0.1:', 'http://localhost:')):
        raise ValueError('Use an isolated loopback lobby service for filming.')
    cast = json.loads(args.cast.read_text())
    root = args.output.resolve(); root.mkdir(parents=True, exist_ok=False)
    cache = root/'render-cache'; cache.mkdir()
    os.environ['FREAK_MELEE_RENDER_CACHE'] = str(cache)
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = args.endpoint
    magic = (args.profile/'Freak Melee/css-layout.bin').read_bytes()[:4]
    mario, falco = [cast['fighters'][key]['id'] for key in ('MarioRemix', 'Falco')]
    room = ''.join(secrets.choice('ABCDEFGHJKLMNPQRSTUVWXYZ23456789') for _ in range(8))
    (root/'room.txt').write_text(room)
    clients = []
    try:
        for i, identities in enumerate(((mario, falco), (falco,))):
            profile = root/f'profile-{i}'; data = profile/'Freak Melee'
            content = data/'content'; content.mkdir(parents=True)
            shutil.copyfile(args.profile/'Freak Melee/content/startup.txt', content/'startup.txt')
            (data/'css-layout.bin').write_bytes(layout(magic, identities))
            for folder, suffix in (('cache', 'fmc'), ('installed', 'fmi')):
                (content/folder).mkdir()
                for identity in identities:
                    shutil.copyfile(Path(cast['content'])/folder/(identity+'.'+suffix), content/folder/(identity+'.'+suffix))
            client = Client(args.exe.resolve(), args.iso.resolve(), profile, root/f'client-{i}',
                            'Twilight' if i == 0 else 'Guest', window_size=(1280,960))
            clients.append(client)
            client.click('nav-1'); client.enter('lobby-code', room); client.click('join-lobby'); client.wait_cards(i+1)
        for client in clients:
            client.wait_cards(2); client.click('lobby-ready'); client.screenshot()
        sender, receiver = clients
        state, text = sender.state()
        tx, ty = map(float, re.search(r'\[css-tile\] index=0 center=([\d.-]+),([\d.-]+)', text).groups())
        sender.move(tx-2.7, ty+2)
        if state['hand'] != 1: sender.tap('b')
        sender.tap('a'); time.sleep(.5)
        cookie, _ = sender.state()
        sender.command('ui-drag', *sender.pixel(cookie['cx']-2.7, cookie['cy']+2), *sender.pixel(0,0))
        time.sleep(.4)
        sender.screenshot(); receiver.screenshot()
        with NativeVideo(sender, root/'send-fighter.mp4'):
            time.sleep(1)
            sender.command('ui-drag', *sender.pixel(tx,ty), *sender.pixel(7.6,-14))
            sender.element('confirm-gift', timeout=10); time.sleep(1.4)
            sender.click('confirm-gift'); time.sleep(2)
            sender.screenshot()
        print('Recorded sender gift', flush=True)
        with NativeVideo(receiver, root/'receive-fighter.mp4'):
            time.sleep(1)
            receiver.click('play-accept')
            deadline = time.monotonic()+30
            while time.monotonic()<deadline:
                if (receiver.data/f'content/installed/{mario}.fmi').exists(): break
                time.sleep(.1)
            else: raise RuntimeError('Gift did not install.')
            time.sleep(2); receiver.screenshot()
        (root/'receipt.json').write_text(json.dumps({'room':room, 'fighter':mario, 'installed':True}, indent=2))
        print('Recorded receipt and installed fighter', flush=True)
    finally:
        for client in clients: client.stop()


if __name__ == '__main__':
    main()
