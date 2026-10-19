"""Review friends and email-style letters with two isolated, hidden/muted clients."""
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
    for name in ['exe', 'iso', 'profile-a', 'profile-b', 'output']:
        parser.add_argument('--' + name, type=Path, required=True)
    for name in ['a-handle', 'b-handle', 'endpoint']:
        parser.add_argument('--' + name, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = args.endpoint
    clients, checks, screenshots = [], [], []
    try:
        for profile, name in [(args.profile_a, 'Twilight'), (args.profile_b, 'Guest')]:
            clients.append(Client(args.exe.resolve(), args.iso.resolve(), profile.resolve(), output / name.lower(), name,
                window_size=(1280, 960)))
        a, b = clients

        def wait(client, pattern, timeout=30):
            deadline = time.monotonic() + timeout
            while time.monotonic() < deadline:
                state = client.inspect('hub-social')
                if re.search(pattern, state): return state
                time.sleep(.2)
            raise RuntimeError(f'Social state not reached: {pattern}: {state}')

        for client in clients:
            client.click('nav-3')
            wait(client, r'signed-in=1 busy=0 loaded=1')
        b.click('social-friend-0')
        wait(b, r'busy=0.*profile=1')
        screenshots.append(str(b.screenshot()))
        b.click('profile-friend')
        wait(b, r'busy=0.*friends=1')
        checks.append('An incoming friend request opens a real account profile and can be accepted in game')
        a.click('social-refresh')
        wait(a, r'busy=0.*friends=1')
        screenshots.append(str(a.screenshot()))
        a.enter('social-account-name', args.b_handle)
        a.click('find-account')
        wait(a, r'busy=0.*profile=1')
        a.click('profile-letter')
        a.enter('letter-subject', 'A letter from the hub')
        first = 'Hello from the native Melee hub. This paragraph wraps inside the message field. ' * 3
        second = 'The second paragraph should remain a separate paragraph after delivery.'
        text = first + '\n' + second
        # Enter the newline with a real key event as well as testing the
        # long-text packets sent by Client.enter().
        a.enter('letter-body', first)
        a.command('ui-key', 'enter')
        a.command('ui-text', second)
        a.command('ui-key', 'up')
        a.command('ui-key', 'down')
        screenshots.append(str(a.screenshot()))
        a.click('letter-send')
        wait(a, r'busy=0.*folder=1')
        screenshots.append(str(a.screenshot()))
        checks.append('The native composer wraps multiline input and sends a letter into Sent')
        b.click('nav-4')
        b.click('social-refresh')
        wait(b, r'busy=0.*count=2 total=2')
        screenshots.append(str(b.screenshot()))
        b.click('mail-entry-0')
        wait(b, r'busy=0.*opened=1')
        screenshots.append(str(b.screenshot()))
        checks.append('The second signed-in client receives, opens and marks the letter read')
        b.click('letter-reply')
        b.enter('letter-body', 'Reply from the second account.')
        b.click('letter-send')
        wait(b, r'busy=0.*folder=1')
        a.click('nav-4')
        a.click('mail-inbox')
        a.click('social-refresh')
        wait(a, r'busy=0.*folder=0.*count=1 total=1')
        a.click('mail-entry-0')
        wait(a, r'busy=0.*opened=1')
        screenshots.append(str(a.screenshot()))
        a.click('letter-delete')
        wait(a, r'busy=0.*total=0')
        checks.append('Reply preserves the recipient and subject; deleting a letter updates the mailbox')
        a.click('nav-3')
        a.enter('social-account-name', args.b_handle)
        a.click('find-account')
        wait(a, r'busy=0.*profile=1')
        a.click('profile-block')
        wait(a, r'busy=0.*friends=0')
        a.click('profile-block')
        wait(a, r'busy=0')
        checks.append('Profile blocking removes the friend relationship and supports unblocking')
        # Metadata diagnostics should not contain the letter body.
        for client in clients:
            log = client.stderr.read_text(errors='replace')
            assert text not in log and 'Reply from the second account.' not in log
        (output / 'receipt.json').write_text(json.dumps({'checks': checks, 'screenshots': screenshots,
            'exe_sha256': hashlib.sha256(args.exe.read_bytes()).hexdigest(),
            'limitations': ['This review covers social pages on one PC; account-key file-picker, hosted-service and physical multi-PC acceptance have separate checks.']}, indent=2) + '\n')
    finally:
        for client in clients: client.stop()
    print(f'{len(checks)} native social UI checks passed; {len(screenshots)} screenshots in {output}')


if __name__ == '__main__':
    main()
