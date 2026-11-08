"""Review default-roster creation, preservation and reuse in hidden/muted native clients."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import time
from review_native_css_room import Client


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'iso', 'fixture', 'output']:
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = 'http://localhost:1'
    client = None
    checks, screens = [], []

    def layout():
        data = (client.data / 'css-layout.bin').read_bytes()
        count = struct.unpack_from('<H', data, 4)[0]
        return data, [(struct.unpack_from('<Q', data, 6+i*48)[0], data[14+i*48:46+i*48].hex()) for i in range(count)]

    def build():
        client.click('defaults-open'); client.element('defaults-confirm')
        client.click('defaults-confirm')
        end = time.monotonic() + 240
        while time.monotonic() < end:
            state = client.inspect('defaults')
            if 'mode=0 completed=24 tiles=24' in state: return
            if 'mode=3' in state: raise RuntimeError(state)
            time.sleep(.15)
        raise RuntimeError('Default creation stalled: ' + state)

    try:
        client = Client(args.exe.resolve(), args.iso.resolve(), args.fixture.resolve(), output / 'create',
            'Default roster review', window_size=(1280, 960), chat_endpoint='http://localhost:1', write_profile=False)
        client.click('nav-5')
        before, old_tiles = layout()
        assert len(old_tiles) == 1
        original_id = old_tiles[0][1]
        client.click('defaults-open'); client.element('defaults-confirm')
        time.sleep(.5); screens.append(str(client.screenshot()))
        client.click('defaults-cancel')
        assert layout()[0] == before
        checks.append('The native empty/full confirmation can be cancelled without changing the CSS')
        build()
        filled, tiles = layout()
        assert len(tiles) == 24 and all(stock == 0 for stock, _ in tiles)
        assert original_id not in {identity for _, identity in tiles}
        assert (client.data / 'content/installed' / (original_id + '.fmi')).exists()
        registry = {int(kind): identity for kind, identity in (line.split() for line in (client.data / 'content/default-roster.txt').read_text().splitlines())}
        assert len(registry) == 24 and set(registry.values()) == {identity for _, identity in tiles}
        # Kirby's internal kind is 4; neither alternate/companion member is a separate tile.
        assert 4 not in registry and len(list((client.data / 'projects').iterdir())) == 25
        for project in (client.data / 'projects').iterdir():
            assert (project / 'project.txt').exists() and (project / 'behavior.fms').stat().st_size > 0
            info = (project / 'project.txt').read_text()
            if info.startswith('title=Donkey Kong\n'): assert 'tile_name=D K\n' in info
            if info.startswith('title=Captain Falcon\n'): assert 'tile_name=C. Falcon\n' in info
        client.click('hub-close'); client.element('play-workshop')
        screens.append(str(client.screenshot()))
        checks.append('The replacement layout contains 24 independent editable source fighters, excludes Kirby and preserves the first fighter in the Workshop')

        # Publish an edited default in the test-owned installation, then ask for
        # defaults again. Its original cached build should become a separate draft.
        data = client.folder / 'appdata'; content = client.data / 'content'
        mario = next(p for p in (client.data / 'projects').iterdir() if (p / 'installed.id').read_text().strip() == registry[0])
        manifest = mario / 'project.txt'
        manifest.write_text(manifest.read_text().replace('title=Mario\n', 'title=Edited Mario\n'), newline='\n')
        client.stop(); client = None
        tool = args.exe.resolve().with_name('native_content_tool.exe')
        env = dict(os.environ, LOCALAPPDATA=str(data))
        def run(*parts):
            result = subprocess.run([str(tool), '--root', str(content), *map(str, parts)],
                env=env, capture_output=True, text=True, creationflags=subprocess.CREATE_NO_WINDOW, timeout=120)
            if result.returncode: raise RuntimeError(result.stdout + result.stderr)
            return result.stdout
        identity = re.search(r'Cached source fighter ([0-9a-f]{64})', run('package-project', mario, mario / 'portrait.png'))[1]
        run('install-project', mario, identity)
        assert identity != registry[0]
        client = Client(args.exe.resolve(), args.iso.resolve(), data, output / 'reuse',
            'Default roster review', window_size=(1280, 960), chat_endpoint='http://localhost:1', write_profile=False)
        client.click('nav-5'); build()
        assert layout()[0] == filled
        assert (client.data / 'content/installed' / (identity + '.fmi')).exists()
        projects = list((client.data / 'projects').iterdir())
        assert len(projects) == 26
        assert any((p / 'project.txt').read_text().startswith('title=Edited Mario\n') for p in projects)
        assert any((p / 'installed.id').read_text().strip() == registry[0] and (p / 'project.txt').read_text().startswith('title=Mario\n') for p in projects)
        checks.append('Restoring defaults keeps a published edited Mario and imports the original baseline as its own editable fighter')
        installed = set((client.data / 'content/installed').glob('*.fmi'))
        build()
        assert layout()[0] == filled and len(list((client.data / 'projects').iterdir())) == 26
        assert set((client.data / 'content/installed').glob('*.fmi')) == installed
        checks.append('Repeating the shortcut reuses its original fighters without duplicating drafts or installed packages')
        client.click('hub-close'); client.element('play-workshop')
        screens.append(str(client.screenshot()))
        (output / 'receipt.json').write_text(json.dumps({'checks': checks, 'screenshots': screens,
            'exe_sha256': hashlib.sha256(args.exe.read_bytes()).hexdigest(),
            'limitations': ['Menu/content review only; this does not play matches with every generated fighter.']}, indent=2) + '\n')
    finally:
        if client: client.stop()
    print(f'{len(checks)} default-roster checks passed')


if __name__ == '__main__':
    main()
