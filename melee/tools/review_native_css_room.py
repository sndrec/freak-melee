"""Review native CSS participant cards and fighter delivery with two clients.

Uses isolated copies of a test profile, a fresh private lobby code, and hidden,
muted clients. No global chat text is sent. Requires the configured room service.
"""
import argparse
import os
from pathlib import Path
import re
import secrets
import shutil
import struct
import subprocess
import time


class Client:
    def __init__(self, exe, iso, profile, folder, name, missing_first=False, live_chat=False,
                 window_size=None, control=None, without_drafts=False, launch_args=()):
        self.folder = folder
        folder.mkdir(parents=True)
        appdata = folder / 'appdata'
        shutil.copytree(profile, appdata)
        self.data = appdata / 'Freak Melee'
        if without_drafts:
            projects = (self.data/'projects').resolve()
            assert projects.is_relative_to(folder.resolve())
            if projects.exists():
                shutil.rmtree(projects)
        if missing_first:
            layout_path = self.data / 'css-layout.bin'
            layout = layout_path.read_bytes()
            count = struct.unpack_from('<H', layout, 4)[0]
            assert count >= 2, 'The fixture needs two independent fighters'
            missing = layout[14:46].hex()
            for relative in [f'content/installed/{missing}.fmi', f'content/cache/{missing}.fmc']:
                (self.data / relative).unlink(missing_ok=True)
            body = layout[:4] + struct.pack('<H', count-1) + layout[54:-4]
            digest = 2166136261
            for byte in body:
                digest = ((digest ^ byte)*16777619) & 0xffffffff
            layout_path.write_bytes(body + struct.pack('<I', digest))
            self.missing = missing
        (appdata / 'Freak Melee/content/hub-profile.txt').write_text(name + '\n\n')
        self.stderr = folder / 'stderr.log'
        self.control = control if control else exe.with_name('melee_native_control.exe')
        self.identity = 'css-room-' + secrets.token_hex(6)
        env = dict(os.environ, LOCALAPPDATA=str(appdata),
                   FREAK_MELEE_CONTROL_ID=self.identity,
                   FREAK_MELEE_CHAT_ENDPOINT='http://127.0.0.1:1')
        if live_chat:
            env.pop('FREAK_MELEE_CHAT_ENDPOINT', None)
        with (folder / 'stdout.log').open('w') as out, self.stderr.open('w') as err:
            self.process = subprocess.Popen(
                [str(exe), '--iso', str(iso), '--hidden', '--mute',*map(str,launch_args)],
                cwd=folder, env=env, stdout=out, stderr=err,
                creationflags=subprocess.CREATE_NO_WINDOW)
        (folder / 'pid.txt').write_text(str(self.process.pid))
        (folder / 'control-id.txt').write_text(self.identity)
        for _ in range(150):
            if self.process.poll() is not None:
                raise RuntimeError(f'Client startup failed: {self.process.returncode}')
            if 'Native control channel ready' in (folder / 'stdout.log').read_text(errors='replace'):
                self.command('hold', 0, 'sx=0')
                if window_size:
                    self.resize(*window_size)
                break
            time.sleep(.1)
        else:
            self.stop()
            raise RuntimeError('Control channel did not open')

    def stop(self):
        if self.process.poll() is None:
            self.process.terminate()
            self.process.wait(timeout=15)

    def resize(self, width, height):
        import ctypes
        from ctypes import wintypes
        user32 = ctypes.WinDLL('user32', use_last_error=True)
        callback = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
        user32.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
        user32.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
        user32.SetWindowPos.argtypes = [wintypes.HWND, wintypes.HWND, ctypes.c_int, ctypes.c_int,
                                       ctypes.c_int, ctypes.c_int, wintypes.UINT]
        user32.EnumWindows.argtypes = [callback, wintypes.LPARAM]
        resized, errors = [], []
        @callback
        def visit(window, _):
            pid = wintypes.DWORD()
            user32.GetWindowThreadProcessId(window, ctypes.byref(pid))
            title = ctypes.create_unicode_buffer(128)
            user32.GetWindowTextW(window, title, len(title))
            if pid.value == self.process.pid and title.value == 'Freak Melee':
                # Resize only our hidden window, preserving position, Z order and focus.
                if not user32.SetWindowPos(window, None, 0, 0, width, height, 0x16):
                    errors.append(ctypes.get_last_error())
                else:
                    resized.append(window)
            return True
        user32.EnumWindows(visit, 0)
        if errors:
            raise ctypes.WinError(errors[0])
        if not resized:
            raise RuntimeError('Test client has no window to resize')

    def command(self, *parts):
        if self.process.poll() is not None:
            raise RuntimeError(f'{self.folder}: client exited {self.process.returncode}')
        subprocess.run([str(self.control), '--id', self.identity, *map(str, parts)],
                       check=True, capture_output=True, creationflags=subprocess.CREATE_NO_WINDOW)

    def inspect(self, element):
        start = self.stderr.stat().st_size
        self.command('ui-inspect', element)
        for _ in range(30):
            time.sleep(.1)
            with self.stderr.open('r', errors='replace') as file:
                file.seek(start)
                text = file.read()
            if re.search(r'\[(?:css-cards|css-selection|hub-inspect|hub-lobbies|ui-inspect|play-menu|quick-play|stadium-state|netplay-state|costume-player|sss-state|room-state|watch-playback)\]', text):
                return text
            if self.process.poll() is not None:
                raise RuntimeError(f'Client exited {self.process.returncode}')
        raise RuntimeError(f'Inspection stalled: {element}')

    def element(self, name, timeout=60):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            try:
                text = self.inspect(name)
            except subprocess.CalledProcessError:
                time.sleep(.2)
                continue
            match = re.search(r'origin=([\d.-]+),([\d.-]+) size=([\d.-]+),([\d.-]+)', text)
            if match:
                x, y, w, h = map(float, match.groups())
                if w > 0 and h > 0:
                    return round(x+w/2), round(y+h/2)
            time.sleep(.15)
        raise RuntimeError(f'{self.folder}: {name} unavailable')

    def click(self, name):
        self.command('ui-click', *self.element(name))
        time.sleep(.3)

    def enter(self, name, text):
        self.click(name)
        self.command('ui-key', 'a', 'ctrl')
        self.command('ui-text', text)
        time.sleep(.15)

    def wait_cards(self, count, timeout=25):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            text = self.inspect('css-cards')
            if f'online=1 count={count}' in text:
                return text
            time.sleep(.2)
        raise RuntimeError(f'{self.folder}: expected {count} room cards; {text}')

    def screenshot(self):
        before = set((self.folder / 'screenshots').glob('*.png'))
        self.command('screenshot')
        for _ in range(150):
            found = set((self.folder / 'screenshots').glob('*.png')) - before
            if found:
                image = sorted(found)[0]
                contents = image.read_bytes()
                if len(contents) > 24 and contents[-8:-4] == b'IEND':
                    self.size = struct.unpack_from('>II', contents, 16)
                    return image
            time.sleep(.1)
        raise RuntimeError('Screenshot did not complete')

    def state(self, port=0):
        text = self.inspect('css-selection')
        match = re.search(r'\[css-slot\] slot='+str(port)+r' kind=(\d+) chosen=(\d+) hand=(\d+) holding=(\d+) '
                          r'hand_pos=([\d.-]+),([\d.-]+) cookie_pos=([\d.-]+),([\d.-]+)', text)
        if not match:
            raise RuntimeError(text)
        return dict(zip(['kind', 'chosen', 'hand', 'holding', 'x', 'y', 'cx', 'cy'], map(float, match.groups()))), text

    def move(self, x, y, port=0):
        for axis, target in [('x', x), ('y', y)]:
            for _ in range(50):
                state, _ = self.state(port)
                error = target - state[axis]
                if abs(error) < .25:
                    break
                magnitude = 80 if abs(error) > 3 else 40 if abs(error) > .8 else 20
                speed = (magnitude*magnitude-200)*.0002
                frames = min(20, max(1, round(abs(error)/speed)))
                self.command('tap', port, f's{axis}={magnitude if error > 0 else -magnitude}', f'frames={frames}')
                time.sleep(frames/60+.1)
            else:
                raise RuntimeError(f'Hand did not reach {axis}={target}')

    def tap(self, key, frames=1, port=0):
        self.command('tap', port, key, f'frames={frames}')
        time.sleep(frames/60+.2)

    def pixel(self, x, y):
        _, text = self.state()
        left, right, top, bottom = map(float, re.search(r'viewport=([\d.-]+),([\d.-]+),([\d.-]+),([\d.-]+)', text).groups())
        w, h = self.size
        gw = min(w, h*4/3)
        gh = gw*.75
        return round((w-gw)/2+(x-left)/(right-left)*gw), round((h-gh)/2+(top-y)/(top-bottom)*gh)

    def canvas(self, x, y):
        w, h = self.size
        scale = min(w/640, h/480)
        return round((w-640*scale)/2+x*scale), round((h-480*scale)/2+y*scale)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'iso', 'profile', 'output']:
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--endpoint', required=True, help='Isolated local signaling service URL')
    parser.add_argument('--control', type=Path, help='Development control tool outside the installation')
    args = parser.parse_args()
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = args.endpoint
    exe, iso, profile, output = [getattr(args, n).resolve() for n in ['exe', 'iso', 'profile', 'output']]
    output.mkdir(exist_ok=False, parents=True)
    clients = []
    room = ''.join(secrets.choice('ABCDEFGHJKLMNPQRSTUVWXYZ23456789') for _ in range(8))
    (output / 'room.txt').write_text(room)
    try:
        for index in range(2):
            client = Client(exe, iso, profile, output / str(index),
                            f'CSS Review {index+1}', missing_first=index == 1,
                            without_drafts=index == 1, window_size=(960,540),
                            control=args.control.resolve() if args.control else None)
            clients.append(client)
            client.click('nav-1')
            client.enter('lobby-code', room)
            client.click('join-lobby')
            client.wait_cards(index+1)
        for index, client in enumerate(clients):
            text = client.wait_cards(2)
            local = re.search(r'\[css-card\] display=0 .*canonical=(\d+) local=(\d+)', text)
            assert local and local.groups() == (str(index), '1'), text
            client.click('lobby-ready')
            client.screenshot()
            state, text = client.state()
            tx, ty = map(float, re.search(r'\[css-tile\] index=0 center=([\d.-]+),([\d.-]+)', text).groups())
            client.move(tx-2.7, ty+2)
            if state['hand'] != 1:
                client.tap('b')
            client.tap('a')
            assert client.state()[0]['chosen'] == 1
            # Free cookies retain their fighter choice, including mouse drags.
            cookie = client.state()[0]
            client.command('ui-drag', *client.pixel(cookie['cx']-2.7, cookie['cy']+2), *client.pixel(0, 0))
            time.sleep(.5)
            assert client.state()[0]['holding'] == 0
            client.screenshot()
        sender, receiver = clients
        for client in clients:
            for _ in range(50):
                text = client.wait_cards(2)
                if len(re.findall(r'\[css-card\].*selected=1', text)) == 2:
                    break
                time.sleep(.1)
            else:
                raise RuntimeError('Provisional CSS choices did not reach the other client')
        # The remote selection arrives before readying or explicit acceptance.
        preview = receiver.data / f'content/cache/{receiver.missing}.fmc'
        assert not (receiver.data / f'content/installed/{receiver.missing}.fmi').exists()
        roster_before_preview = (receiver.data / 'css-layout.bin').read_bytes()
        for _ in range(300):
            cards = receiver.inspect('css-cards')
            if preview.exists() and re.search(r'\[css-card\].*local=0.*selected=1.*portrait=1', cards):
                break
            time.sleep(.1)
        else:
            raise RuntimeError('Selected remote fighter was not cached and displayed before readying')
        assert not (receiver.data / f'content/installed/{receiver.missing}.fmi').exists()
        assert (receiver.data / 'css-layout.bin').read_bytes() == roster_before_preview
        receiver.screenshot()
        _, text = sender.state()
        tx, ty = map(float, re.search(r'\[css-tile\] index=0 center=([\d.-]+),([\d.-]+)', text).groups())
        sender.command('ui-drag', *sender.pixel(tx, ty), *sender.pixel(7.6, -14))
        sender.element('confirm-gift', timeout=10)
        sender.screenshot()
        sender.tap('b')
        assert 'absent' in sender.inspect('confirm-gift')
        # Receive an uncached fighter while spectating and running local practice.
        receiver.tap('b', 40)
        receiver.click('nav-1')
        for _ in range(2):
            receiver.command('ui-click', *receiver.canvas(406, 194.5))
            time.sleep(.6)
        assert 'local=1' not in receiver.wait_cards(1)
        sender.wait_cards(1)
        receiver.click('lobby-ready')
        if receiver.state()[0]['holding']:
            receiver.tap('a')
        receiver.tap('start')
        for _ in range(200):
            if '[css-selection] practice' in receiver.inspect('css-selection'):
                break
            time.sleep(.1)
        else:
            raise RuntimeError('Receiver did not enter ready practice')
        # Evict only this test fixture's automatic cache to separately test
        # an uncached gift while the recipient is practicing as a spectator.
        preview.unlink()
        sender.command('ui-click', *sender.pixel(tx, ty))
        time.sleep(.3)
        sender.tap('x')
        sender.click('gift-recipient')
        assert 'enabled=1' in sender.inspect('confirm-gift')
        sender.click('confirm-gift')
        sender.wait_cards(1)
        receiver.element('play-accept', timeout=15)
        receiver.screenshot()
        installed = set((receiver.data / 'content/installed').glob('*.fmi'))
        roster_count = struct.unpack_from('<H', (receiver.data / 'css-layout.bin').read_bytes(), 4)[0]
        assert not (receiver.data / f'content/cache/{receiver.missing}.fmc').exists()
        receiver.click('play-accept')
        for _ in range(200):
            if ((receiver.data / f'content/installed/{receiver.missing}.fmi').exists() and
                struct.unpack_from('<H', (receiver.data / 'css-layout.bin').read_bytes(), 4)[0] == roster_count+1):
                break
            time.sleep(.1)
        else:
            raise RuntimeError('The offered package was not received and installed')
        assert len(list((receiver.data / 'content/installed').glob('*.fmi'))) == len(installed)+1
        assert struct.unpack_from('<H', (receiver.data / 'css-layout.bin').read_bytes(), 4)[0] == roster_count+1
        receiver.screenshot()
        receiver.tap('start')
        receiver.command('tap', 0, 'l', 'r', 'a', 'start', 'frames=2')
        receiver.click('nav-5')
        receiver.click('local-practice')
        receiver.wait_cards(1)
        receiver.screenshot()
        receiver.tap('b', 40)
        receiver.click('nav-1')
        receiver.command('ui-click', *receiver.canvas(406, 194.5))
        receiver.wait_cards(2)
        sender.wait_cards(2)
        # Returning to Fighting restores gameplay connections. The next gift
        # remains on its independent content channel and uses the card drop.
        sender.command('ui-drag', *sender.pixel(tx, ty), *sender.pixel(7.6, -14))
        sender.click('confirm-gift')
        receiver.click('nav-5')
        receiver.click('accept-fighter')
        time.sleep(1)
        assert len(list((receiver.data / 'content/installed').glob('*.fmi'))) == len(installed)+1
        assert struct.unpack_from('<H', (receiver.data / 'css-layout.bin').read_bytes(), 4)[0] == roster_count+1
        receiver.screenshot()
        # The recipient has packages but no authoring projects. Retire the
        # sender and its test-only draft directory before editing either one.
        sender.stop()
        author_projects = (sender.data/'projects').resolve()
        assert author_projects.is_relative_to(sender.folder.resolve())
        author_projects.rename(sender.data/'author-projects-offline')
        assert not list((receiver.data/'projects').glob('fighter-*'))
        receiver.click('nav-1')
        receiver.click('leave-lobby')
        receiver.click('nav-5')
        for index in range(2):
            receiver.command('ui-click', *receiver.canvas(213.5+57*index, 186.5))
            receiver.click('edit-fighter')
            receiver.element('workshop-editor-header')
            deadline = time.monotonic()+60
            while time.monotonic() < deadline:
                if 'text=Draft preview ready.' in receiver.inspect('workshop-editor-status'):
                    break
            else:
                raise RuntimeError('Received package did not open in the frozen preview')
            receiver.click('workshop-mode-script')
            receiver.click('workshop-script-validate')
            deadline = time.monotonic()+30
            while time.monotonic() < deadline:
                if 'text=Source and fighter assets validated.' in receiver.inspect('workshop-script-status'):
                    break
            else:
                raise RuntimeError('Received package source check did not finish')
            receiver.screenshot()
            receiver.click('workshop-back-drafts')
            receiver.element('edit-fighter')
        drafts = list((receiver.data/'projects').glob('fighter-*'))
        assert len(drafts) == 2
        assert any((draft/'installed.id').read_text().strip() == receiver.missing for draft in drafts)
        assert all((draft/'behavior.fms').stat().st_size > 1000 for draft in drafts)
        (output / 'result.txt').write_text('PASS: early remote package cache and portrait without roster installation; two native clients, local-first canonical identities, provisional choices, free cookies, mouse tile drop, cancel, uncached spectator delivery during practice, roster insertion, spectator card exclusion, role reconnection and duplicate-install prevention.\nPASS: received fighters opened as independent Workshop drafts and passed source validation with the author offline and original test draft paths absent.\n')
        print((output / 'result.txt').read_text().strip())
    finally:
        for client in clients:
            client.stop()


if __name__ == '__main__':
    main()
