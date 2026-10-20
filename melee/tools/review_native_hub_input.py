"""Check held-button input ownership in a hidden, muted native hub."""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import re
import time
from review_native_css_room import Client


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'iso', 'profile', 'output']:
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = 'http://localhost:1'
    client = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(), output / 'client',
                    'Input review', window_size=(1280, 960), chat_endpoint='http://localhost:1')
    checks, screenshots = [], []

    def state():
        text = client.inspect('hub-input')
        if '[ui-inspect] hub-input: absent' in text:
            # The shell routes hub diagnostics only while the hub is visible.
            return {'visible': 0}
        match = re.search(r'page=(\d+) visible=(\d+) listed=(\d+) players=(\d+) field=(\d+) back-frames=(\d+)', text)
        if not match: raise RuntimeError(text)
        return dict(zip(['page', 'visible', 'listed', 'players', 'field', 'back'], map(int, match.groups())))

    def expect(**wanted):
        deadline = time.monotonic() + 15
        while time.monotonic() < deadline:
            actual = state()
            if all(actual[key] == value for key, value in wanted.items()): return actual
            time.sleep(.1)
        raise AssertionError((wanted, actual))

    def move(element):
        client.command('ui-move', *client.element(element))

    def hold(button):
        client.command('hold', 0, *button.split('+') if button else ['sx=0'])

    def release():
        hold('')
        time.sleep(.15)

    def stable(seconds=.8, **wanted):
        deadline = time.monotonic() + seconds
        while time.monotonic() < deadline:
            actual = state()
            assert all(actual[key] == value for key, value in wanted.items()), (wanted, actual)
            time.sleep(.08)

    def key(vk, down=True, repeat=False):
        # Deliver Windows key messages only to this test's hidden window.
        # This leaves the user's foreground window and keyboard untouched.
        from ctypes import wintypes
        user32 = ctypes.WinDLL('user32', use_last_error=True)
        callback = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
        user32.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
        user32.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
        user32.EnumWindows.argtypes = [callback, wintypes.LPARAM]
        windows = []
        @callback
        def visit(window, _):
            pid = wintypes.DWORD()
            user32.GetWindowThreadProcessId(window, ctypes.byref(pid))
            title = ctypes.create_unicode_buffer(128)
            user32.GetWindowTextW(window, title, len(title))
            if pid.value == client.process.pid and title.value == 'Freak Melee': windows.append(window)
            return True
        user32.EnumWindows(visit, 0)
        assert len(windows) == 1
        user32.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
        flags = 1 | (user32.MapVirtualKeyW(vk, 0) << 16)
        if repeat or not down: flags |= 1 << 30
        if not down: flags |= 1 << 31
        assert user32.PostMessageW(windows[0], 0x100 if down else 0x101, vk, flags)
        time.sleep(.12)

    def tap_value():
        text = client.inspect('profile-tap')
        match = re.search(r'\[ui-inspect-value\].*text=(On|Off)', text)
        if not match: raise RuntimeError(text)
        return match[1]

    try:
        for port in range(4): client.command('hold', port, 'sx=0')
        client.click('nav-1'); client.click('create-lobby')
        expect(page=12, visible=1, listed=1)
        move('list-lobby'); hold('A')
        expect(listed=0); stable(listed=0)
        release(); hold('A'); expect(listed=1); stable(listed=1); release()
        checks.append('Holding A toggles listing once; release and a second press toggle it once again')

        client.click('list-lobby'); expect(listed=0)
        client.click('list-lobby'); expect(listed=1)
        client.command('ui-move', 25, 850); hold('A'); time.sleep(.15)
        move('list-lobby'); stable(listed=1); release()
        checks.append('Mouse toggles remain usable; moving onto a control while already holding A does not activate it')

        client.click('lobby-title'); expect(field=1)
        move('list-lobby'); hold('A+B')
        expect(field=0); stable(page=12, listed=1, field=0); release()
        hold('B'); expect(page=1); stable(page=1); release()
        checks.append('B dismisses text editing once, wins over simultaneous A, and returns one page per new press')

        client.click('nav-6'); client.click('your-profile')
        expect(page=7)
        hold('B'); expect(page=6); stable(page=6); release()
        checks.append('Holding B on a nested profile page returns to Settings once')

        client.click('nav-1'); hold('START'); expect(visible=0); release()
        client.element('play-lobby'); client.click('play-lobby'); expect(visible=1)
        hold('B'); expect(visible=0); stable(seconds=1.0, visible=0); release()
        hold('B'); expect(visible=1); stable(visible=1); release()
        checks.append('B used to close the hub cannot reopen it until released; a fresh B hold opens it once')
        client.click('nav-6'); client.click('controls'); client.click('profile-new')
        client.click('profile-tap'); before = tap_value()
        key(0x0D)
        after = tap_value(); assert after != before
        for _ in range(8): key(0x0D, repeat=True)
        assert tap_value() == after
        key(0x0D, down=False); key(0x0D); key(0x0D, down=False)
        assert tap_value() == before
        checks.append('SDL keyboard auto-repeat activates a focused Controls button once per Enter press')
        client.click('profile-bind'); client.click('bind-0'); key(0x1B)
        for _ in range(8): key(0x1B, repeat=True)
        assert 'visible=1' in client.inspect('profile-close')
        key(0x1B, down=False); key(0x1B); key(0x1B, down=False)
        expect(visible=1, page=6)
        checks.append('Escape cancels binding capture once; repeats do not also close Controls')
        screenshots.append(str(client.screenshot()))
        (output / 'receipt.json').write_text(json.dumps({
            'checks': checks, 'screenshots': screenshots,
            'exe_sha256': hashlib.sha256(args.exe.read_bytes()).hexdigest(),
            'limitations': ['Virtual controller samples use the real native input/menu pipeline; physical adapter capture remains deferred.',
                            'CSS edge consumption has source review; Quick Play cancellation has separate input ownership tests.'],
        }, indent=2) + '\n')
    finally:
        client.stop()
    print(f'{len(checks)} native hub input checks passed')


if __name__ == '__main__':
    main()
