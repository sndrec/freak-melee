"""Review social-page editing and pagination in one hidden, muted client.

The local service fixture supplies nine contacts and nine received letters.
No player profile or public service is used.
"""
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
    for name in ['exe', 'iso', 'profile', 'output']:
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--lookup-handle', required=True)
    parser.add_argument('--endpoint', required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = args.endpoint
    client = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(),
                    output / 'client', 'Paging reviewer', window_size=(1280, 960))
    checks, screenshots = [], []

    def wait(pattern, timeout=30, tag='hub-social'):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            state = client.inspect(tag)
            if re.search(pattern, state):
                return state
            time.sleep(.2)
        raise RuntimeError(f'Social state did not reach {pattern}: {state}')

    try:
        # The preceding invitation review intentionally signs this account out.
        # Its original fixture session is revoked too; use its retained key.
        client.click('nav-6')
        client.click('settings-category-3')
        client.element('settings-back')
        account = wait(r'busy=0 signed-in=[01]', tag='hub-account')
        if 'signed-in=0' in account:
            client.click('account-submit')
            wait(r'busy=0 signed-in=1', tag='hub-account')
            checks.append('The saved account key signs back in after the preceding session was revoked')
        client.click('nav-3')
        initial = wait(r'busy=0.*loaded=1.*friends=9.*friends-offset=0 friend-count=8')
        revision = int(re.search(r' revision=(\d+)', initial)[1])
        client.enter('social-account-name', args.lookup_handle[:-1])
        # Wait for an actual background response, rather than relying solely
        # on a sleep spanning the refresh interval. Keep the field focused.
        deadline = time.monotonic() + 25
        while time.monotonic() < deadline:
            state = client.inspect('hub-social')
            current = re.search(r' revision=(\d+)', state)
            if 'busy=0' in state and current and int(current[1]) > revision:
                break
            time.sleep(.2)
        else:
            raise RuntimeError('No periodic social refresh completed')
        client.command('ui-text', args.lookup_handle[-1:])
        client.command('ui-key', 'enter')
        wait(r'busy=0.*profile=1 profile-handle=' + re.escape(args.lookup_handle) + r' ')
        checks.append('Typing and Enter-to-search survive a completed background refresh')

        client.command('ui-key', 'escape')
        client.click('social-next')
        wait(r'busy=0.*friends=9.*friends-offset=8 friend-count=1')
        screenshots.append(str(client.screenshot()))
        client.click('social-friend-0')
        wait(r'busy=0.*profile=1')
        client.click('profile-friend')
        wait(r'busy=0.*friends=8')
        client.command('ui-key', 'escape')
        wait(r'busy=0.*friends=8.*friends-offset=0 friend-count=8')
        screenshots.append(str(client.screenshot()))
        checks.append('Removing the sole contact on page two returns Friends to its last populated page')

        client.click('nav-4')
        client.click('mail-inbox')
        wait(r'busy=0.*folder=0 offset=0 count=8 total=9')
        client.click('social-next')
        wait(r'busy=0.*folder=0 offset=8 count=1 total=9')
        screenshots.append(str(client.screenshot()))
        client.click('mail-entry-0')
        wait(r'busy=0.*opened=1')
        client.click('letter-delete')
        wait(r'busy=0.*folder=0 offset=0 count=8 total=8')
        screenshots.append(str(client.screenshot()))
        checks.append('Deleting the sole letter on page two returns Inbox to its last populated page')
        (output / 'receipt.json').write_text(json.dumps({
            'checks': checks, 'screenshots': screenshots,
            'exe_sha256': hashlib.sha256(args.exe.read_bytes()).hexdigest(),
            'limitations': ['Local fixture on one PC; physical controller and visual acceptance remain separate.'],
        }, indent=2) + '\n')
    finally:
        client.stop()
    print(f'{len(checks)} native social editing/paging checks passed; {len(screenshots)} screenshots in {output}')


if __name__ == '__main__':
    main()
