"""Check the native global player directory without posting a chat message.

Starts one hidden, muted client with an isolated profile and the configured
service endpoint. Only joins, identifies the client, reads the directory and
captures a screenshot. No message or invitation is sent.
"""
import argparse
from pathlib import Path
from review_native_css_room import Client


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'iso', 'profile', 'output']:
        parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    exe, iso, profile, output = [getattr(args, n).resolve()
                                for n in ['exe', 'iso', 'profile', 'output']]
    output.mkdir(exist_ok=False, parents=True)
    client = Client(exe, iso, profile, output / 'client', 'Quiet Hub Review', live_chat=True)
    try:
        client.click('nav-2')
        client.element('chat-player', timeout=30)
        client.screenshot()
        (output / 'result.txt').write_text(
            'PASS: native Chatroom receives the live player directory. No chat messages sent.\n')
        print((output / 'result.txt').read_text().strip())
    finally:
        client.stop()


if __name__ == '__main__':
    main()
