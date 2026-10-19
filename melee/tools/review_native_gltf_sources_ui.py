"""Check source-only import history in the real Workshop, hidden and muted.

An unchanged mesh is imported from a relocated file. The four core assets remain
byte-identical while its source link changes; Undo/Redo must own that metadata
change, and importing the same file again must leave history unchanged.
"""
import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import time
from pathlib import Path

from review_native_css_room import Client
from review_native_skin_weights import make_glb

ASSETS = ('fighter.dat', 'animation.dat', 'costume.dat', 'assets.fma')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'tool', 'control', 'iso', 'draft', 'output'):
        parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    profile = output/'profile'
    draft = profile/'Freak Melee/projects/source-history'
    shutil.copytree(args.draft.resolve(), draft)
    (profile/'Freak Melee/content').mkdir()
    first, relocated = output/'original.glb', output/'relocated.glb'
    make_glb(first)
    shutil.copyfile(first, relocated)
    receipt = dict(status='RUNNING', cases=[])
    original = {name: hashlib.sha256((args.draft/name).read_bytes()).hexdigest() for name in ASSETS}
    client = None

    def tool(*parts):
        result = subprocess.run([str(args.tool.resolve()), *map(str, parts)],
            capture_output=True, text=True, encoding='utf-8', errors='replace',
            env=dict(os.environ, LOCALAPPDATA=str(output/'tool-profile')),
            creationflags=subprocess.CREATE_NO_WINDOW)
        assert result.returncode == 0, result.stdout+result.stderr
        return result.stdout

    tool('project-model-import', draft, first, 0, 0, 'add', 1, 'auto')
    # Establish the compactor's fixed point before checking metadata-only edits.
    for _ in range(3):
        previous = (draft/'costume.dat').read_bytes()
        tool('project-model-import', draft, first, 0, 0, 0, 1, 'auto')
        if (draft/'costume.dat').read_bytes() == previous:
            break
    else:
        raise AssertionError('Identical geometry did not reach a stable saved layout')

    def ready(c):
        start = (c.folder/'stdout.log').stat().st_size
        deadline = time.monotonic()+60
        while time.monotonic() < deadline:
            c.command('ui-inspect', 'workshop-preview')
            time.sleep(.15)
            with (c.folder/'stdout.log').open(errors='replace') as log:
                log.seek(start)
                text = log.read()
            if 'editing=1 live=0 transitioning=0' in text and '[workshop-camera] active=1' in text:
                return
        raise RuntimeError('Preview did not become ready')

    def launch(folder):
        c = Client(args.exe.resolve(), args.iso.resolve(), profile, output/folder,
            'Source History Review', window_size=(1440, 900), control=args.control.resolve())
        try:
            c.click('nav-5')
            c.click('edit-fighter')
            c.element('workshop-editor-header')
            ready(c)
            c.click('workshop-mode-model')
            return c
        except BaseException:
            c.stop()
            raise

    def save(c):
        c.click('workshop-save-top')
        assert 'Draft saved.' in c.inspect('workshop-save-status'), 'Draft Save did not complete'
        path = c.data/'projects/source-history'
        return {name: (path/name).read_bytes() for name in ASSETS}

    def reimport(c):
        c.click('workshop-model-import')
        c.enter('workshop-model-import-path', str(relocated))
        c.click('workshop-model-import-load')
        c.click('workshop-model-import-next')
        c.click('workshop-model-import-target')
        c.command('ui-key', 'home')
        c.command('ui-key', 'down')
        c.command('ui-key', 'enter')
        assert 'value=0 text=' in c.inspect('workshop-model-import-target')
        c.click('workshop-model-import-next')
        c.click('workshop-model-import-next')
        ready(c)

    def source_path(c):
        return tool('project-model-sources', c.data/'projects/source-history')

    try:
        client = launch('editing')
        baseline = save(client)
        assert 'original.glb' in source_path(client)
        reimport(client)
        assert save(client) == baseline
        assert 'relocated.glb' in source_path(client)
        client.click('workshop-undo')
        assert save(client) == baseline
        assert 'original.glb' in source_path(client)
        client.click('workshop-redo')
        assert save(client) == baseline
        assert 'relocated.glb' in source_path(client)
        receipt['cases'].append('Metadata-only import has complete Undo/Redo without changing core assets')
        reimport(client)
        assert save(client) == baseline
        client.click('workshop-undo')
        assert 'original.glb' in source_path(client), 'A no-op import inserted an empty history entry'
        client.click('workshop-redo')
        assert 'relocated.glb' in source_path(client)
        receipt['cases'].append('Repeated no-op import retains the existing history entry')
        receipt['screenshot'] = str(client.screenshot())
        shutil.copytree(client.data/'projects/source-history', draft, dirs_exist_ok=True)
        client.stop()
        client = launch('reopened')
        assert save(client) == baseline
        assert 'relocated.glb' in source_path(client)
        receipt['cases'].append('Saved source metadata reopens in the native client')
        receipt['status'] = 'PASS'
    except BaseException as error:
        receipt['status'] = 'FAIL'
        receipt['error'] = repr(error)
        raise
    finally:
        if client:
            client.stop()
        assert original == {name: hashlib.sha256((args.draft/name).read_bytes()).hexdigest() for name in ASSETS}
        receipt['source_unchanged'] = True
        receipt['exe_sha256'] = hashlib.sha256(args.exe.read_bytes()).hexdigest()
        (output/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print('PASS: source-only native import history, no-op handling and saved metadata reopen')


if __name__ == '__main__':
    main()
