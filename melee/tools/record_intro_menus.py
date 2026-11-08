"""Record the native first-fighter carousel and appearance setting."""
import argparse
import os
from pathlib import Path
import time

from capture_native_video import NativeVideo
from review_native_css_room import Client


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'iso', 'output'):
        parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    root = args.output.resolve();root.mkdir(parents=True, exist_ok=False)
    cache = root/'render-cache';cache.mkdir()
    os.environ['FREAK_MELEE_RENDER_CACHE'] = str(cache)
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = 'http://127.0.0.1:1'
    for shot in ('carousel', 'settings'):
        profile = root/(shot+'-profile'); content = profile/'Freak Melee/content'
        content.mkdir(parents=True)
        (content/'startup.txt').write_text('8 0 0\n' if shot=='carousel' else '12 0 1\n')
        c = None
        try:
            c = Client(args.exe.resolve(),args.iso.resolve(),profile,root/shot,'Twilight',window_size=(1280,960))
            if shot=='carousel':
                c.element('create-clone');time.sleep(2)
                with NativeVideo(c,root/'choose-fighter.mp4'):
                    time.sleep(1)
                    c.click('setup-fighter-next');time.sleep(1)
                    c.click('setup-fighter-next');time.sleep(1)
                    c.click('setup-fighter-prev');time.sleep(.8)
                    c.click('setup-fighter-prev');time.sleep(1.2)
                    c.click('create-clone');time.sleep(2)
                    c.screenshot()
            else:
                c.click('nav-6');c.element('settings-appearances');time.sleep(.8)
                with NativeVideo(c,root/'appearance-setting.mp4'):
                    time.sleep(1)
                    c.click('settings-appearances');time.sleep(1.8)
                    c.screenshot()
            print('Recorded '+shot,flush=True)
        finally:
            if c:c.stop()


if __name__=='__main__':main()
