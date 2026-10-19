"""Open a large imported mesh in an isolated, hidden and muted Workshop.

Optionally import a source GLB through the actual dialog first. Captures the
full UI and checks the rendered model's editable surface. Only copied drafts
are saved. No chat or multiplayer service is contacted.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import time

from review_native_css_room import Client
from review_native_large_gltf import hashes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'iso', 'profile', 'draft', 'output'):
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--source', type=Path)
    parser.add_argument('--binding', choices=('rigid', 'auto', 'source'), default='rigid')
    parser.add_argument('--vertices', type=int, default=70002)
    args = parser.parse_args()
    original = hashes(args.draft)
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    profile = output/'profile'
    shutil.copytree(args.profile.resolve(), profile)
    projects = (profile/'Freak Melee/projects').resolve()
    assert projects.is_relative_to(output)
    if projects.exists():
        shutil.rmtree(projects)
    shutil.copytree(args.draft.resolve(), projects/'large-review')
    client = None
    receipt = dict(status='RUNNING', imported=bool(args.source), vertices=args.vertices,
                   exe_sha256=hashlib.sha256(args.exe.read_bytes()).hexdigest())

    def wait_inspect(element, predicate, timeout=120):
        deadline = time.monotonic()+timeout
        while time.monotonic() < deadline:
            try:
                text = client.inspect(element)
                if predicate(text):
                    return text
            except RuntimeError as error:
                if 'Inspection stalled' not in str(error):
                    raise
            time.sleep(.15)
        raise RuntimeError('Timed out waiting for '+element)

    def select(element, value):
        client.click(element)
        state = client.inspect(element)
        values = re.findall(r'\[ui-inspect-option\] '+re.escape(element)+r' index=\d+ value=(\S+) text=', state)
        current = re.search(r'\[ui-inspect-value\] '+re.escape(element)+r':.* value=(\S+) text=', state)[1]
        delta = values.index(value)-values.index(current)
        for _ in range(abs(delta)):
            client.command('ui-key', 'down' if delta > 0 else 'up')
        client.command('ui-key', 'enter')
        assert 'value='+value+' text=' in client.inspect(element)

    try:
        client = Client(args.exe.resolve(), args.iso.resolve(), profile,
                        output/'client', 'Large model review', window_size=(1440, 900))
        client.click('nav-5')
        client.click('import-fighter')
        client.click('workshop-draft-0')
        wait_inspect('workshop-editor-header', lambda text: 'visible=1' in text)
        client.click('workshop-mode-model')
        if args.source:
            client.click('workshop-model-context-meshes')
            client.click('workshop-model-import')
            client.enter('workshop-model-import-path', str(args.source.resolve()))
            client.click('workshop-model-import-load')
            client.click('workshop-model-import-next')
            client.click('workshop-model-import-next')
            select('workshop-model-import-binding', args.binding)
            client.click('workshop-model-import-next')
            wait_inspect('workshop-model-import-overlay', lambda text: 'visible=0' in text)
            client.click('workshop-save-top')
            wait_inspect('workshop-save-status', lambda text: 'Draft saved.' in text)
        client.click('workshop-model-context-skinning')
        # The surface report contains all vertices and can take longer than a
        # normal small-element inspection. Read it once without resending it.
        start = client.stderr.stat().st_size
        client.command('ui-inspect', 'workshop-model-surface')
        deadline = time.monotonic()+60
        while time.monotonic() < deadline:
            if client.process.poll() is not None:
                raise RuntimeError('Client exited '+str(client.process.returncode))
            with client.stderr.open(errors='replace') as log:
                log.seek(start)
                text = log.read()
            if 'ready=1 vertices='+str(args.vertices) in text:
                break
            time.sleep(.2)
        else:
            raise RuntimeError('Editable surface did not become ready')
        receipt.update(status='PASS', screenshot=str(client.screenshot()))
    except Exception as error:
        receipt.update(status='FAIL', error=str(error))
    finally:
        if client:
            client.stop()
        receipt['source_unchanged'] = hashes(args.draft) == original
        if not receipt['source_unchanged']:
            receipt.update(status='FAIL', error='The source draft changed')
        (output/'receipt.json').write_text(json.dumps(receipt, indent=2), encoding='utf-8')
    print(json.dumps(receipt, indent=2))
    assert receipt['status'] == 'PASS', receipt


if __name__ == '__main__':
    main()
