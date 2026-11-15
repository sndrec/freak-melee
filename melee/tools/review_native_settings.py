"""Review categorized native Options navigation and persistence in hidden, muted clients."""
import argparse
import json
import os
from pathlib import Path
import re
import time
from review_native_css_room import Client
from review_native_release import close_window


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'iso', 'profile', 'output'):
        parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    for service in ('SERVICE', 'CHAT', 'COMMUNITY'):
        os.environ['FREAK_MELEE_'+service+'_ENDPOINT'] = 'http://127.0.0.1:1'
    os.environ['FREAK_MELEE_MENU_MOTION'] = '0'
    result = {'status': 'RUNNING', 'screenshots': {}, 'checks': []}
    client = None

    def shot(name):
        result['screenshots'][name] = str(client.screenshot())

    def options():
        text = client.inspect('client-options')
        match = re.search(r'fullscreen=(\d+) vsync=(\d+) mono=(\d+) balance=(-?\d+) page=(\d+)',text)
        assert match, text
        return tuple(map(int,match.groups()))

    try:
        client = Client(args.exe.resolve(),args.iso.resolve(),args.profile.resolve(),args.output/'client',
                        'Settings review',window_size=(1440,900))
        client.click('nav-6')
        shot('options-root')
        for index in range(5):
            client.element('settings-category-'+str(index))
        client.click('settings-category-0')
        before = options()
        client.command('ui-move',*client.element('video-vsync'))
        client.tap('A',frames=40)
        after = options()
        assert after[1] == 1-before[1],(before,after)
        shot('video')
        client.tap('B',frames=40)
        assert options()[4] == 6
        client.element('settings-category-1')
        client.click('settings-category-1')
        client.command('ui-move',*client.element('audio-channels'))
        client.tap('A',frames=40)
        assert options()[2] == 1-before[2]
        client.click('audio-music')
        assert options()[3] == min(before[3]+5,100)
        shot('audio')
        client.click('settings-back')
        client.click('settings-category-2')
        client.element('settings-appearances')
        shot('game')
        client.click('settings-back')
        client.click('settings-category-3')
        client.element('settings-account')
        shot('online')
        client.click('settings-back')
        client.click('settings-category-4')
        client.click('local-players-3')
        assert 'players=3' in client.inspect('hub-input')
        shot('controls')
        client.resize(960,540)
        shot('controls-narrow')
        client.click('controls-back')
        shot('options-narrow')
        client.click('settings-back')
        client.element('news-ticker')
        expected = options()[:4]
        result['checks'] += ['five native categories','held A toggles once','held B returns one level',
                             'audio balance','controls local count','return to hub','two window sizes']
        close_window(client.process.pid)
        client.process.wait(timeout=20)
        saved = (client.data/'content/client-options.txt').read_text()
        assert 'vsync '+str(expected[1]) in saved
        profile = client.folder/'appdata'
        client = Client(args.exe.resolve(),args.iso.resolve(),profile,args.output/'reopened',
                        'Settings review',window_size=(960,540))
        client.click('nav-6')
        assert options()[:4] == expected,(options(),expected)
        client.click('settings-category-4')
        assert 'players=3' in client.inspect('hub-input')
        result['checks'].append('settings and local-player count survive restart')
        result['status'] = 'PASS'
    finally:
        if client and client.process.poll() is None:
            close_window(client.process.pid)
            client.process.wait(timeout=20)
        (args.output/'review.json').write_text(json.dumps(result,indent=2))
    print('PASS categorized native Options review')


if __name__ == '__main__':
    main()
