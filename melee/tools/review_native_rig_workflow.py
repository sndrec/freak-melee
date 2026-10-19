"""Review structural bone editing in a copied draft using a hidden, muted client.

Exercises the real controls, complete-asset Save/Undo/Redo, retained temporary
poses, reopening, and Test Character. The source draft remains unchanged.
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
from review_native_skin_weights import make_glb, raw_mesh

ASSETS = ('fighter.dat', 'animation.dat', 'costume.dat', 'assets.fma')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'tool', 'control', 'iso', 'draft', 'output'):
        parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    profile = output/'profile'
    draft = profile/'Freak Melee/projects/rig-workflow'
    shutil.copytree(args.draft.resolve(), draft)
    (profile/'Freak Melee/content').mkdir()
    source_hashes = {name: hashlib.sha256((args.draft/name).read_bytes()).hexdigest() for name in ASSETS}
    receipt = dict(status='RUNNING', cases=[], screenshots=[])
    client = None

    def command(*parts):
        result = subprocess.run([str(args.tool.resolve()), *map(str, parts)],
            env=dict(os.environ, LOCALAPPDATA=str(output/'tool-profile')),
            capture_output=True, text=True, creationflags=subprocess.CREATE_NO_WINDOW)
        assert result.returncode == 0, result.stdout+result.stderr
        return result.stdout

    source = output/'weights.glb'
    make_glb(source)
    command('project-model-import', draft, source, 0, 0, 'add', 1, 'auto')

    def launch(folder):
        c = Client(args.exe.resolve(), args.iso.resolve(), profile, output/folder,
                   'Bone Editing Review', window_size=(1440, 900), control=args.control.resolve())
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
        raise RuntimeError('Preview did not become ready: '+text[-2000:])

    def model(c):
        start = c.stderr.stat().st_size
        c.command('ui-inspect', 'workshop-model-surface')
        for _ in range(40):
            time.sleep(.1)
            with c.stderr.open(errors='replace') as log:
                log.seek(start)
                text = log.read()
            header = re.search(r'\[workshop-model\].*joint=(\d+).*joints=(\d+) revision=(\d+)', text)
            if header:
                poses = {}
                for row in re.finditer(r'\[workshop-model-pose\] index=(\d+) offset=(\d+) selected=(\d+) apply=(\d+) local=([^\n]+)', text):
                    poses[int(row[2])] = dict(index=int(row[1]), selected=int(row[3]),
                        apply=int(row[4]), local=tuple(map(float, row[5].strip().strip(',').split(','))))
                return dict(selected=int(header[1]), count=int(header[2]), revision=int(header[3]), poses=poses)
        raise RuntimeError('Model inspection failed: '+text)

    def click(c, name, panel):
        def rect(element):
            text = c.inspect(element)
            return tuple(map(float, re.search(r'origin=([\d.-]+),([\d.-]+) size=([\d.-]+),([\d.-]+)', text).groups()))
        for _ in range(20):
            x, y, w, h = rect(panel)
            _, ty, _, th = rect(name)
            if ty >= y+4 and ty+th <= y+h-4:
                c.click(name)
                return
            # Use the real scrollbar track to page the containing panel.
            c.command('ui-click', round(x+w+{'workshop-model-inspector': 10, 'workshop-editor-browser': 6, 'workshop-editor-inspector': 6}.get(panel, -3)), round(y+8 if ty < y+4 else y+h-8))
            time.sleep(.15)
        raise RuntimeError('Could not scroll to '+name)

    def save(c):
        c.click('workshop-save-top')
        path = c.data/'projects/rig-workflow'
        return {name: (path/name).read_bytes() for name in ASSETS}

    def choose(c, value):
        text = c.inspect('workshop-model-bone-parent')
        options = re.findall(r'index=(\d+) value=(\d+) text=', text)
        index = next(int(i) for i, v in options if int(v) == value)
        click(c, 'workshop-model-bone-parent', 'workshop-model-inspector')
        c.command('ui-key', 'home')
        for _ in range(index):
            c.command('ui-key', 'down')
        c.command('ui-key', 'enter')
        assert f'value={value} text=' in c.inspect('workshop-model-bone-parent'), 'Wrong parent selected'

    def round_trip(c, before, after, label):
        c.click('workshop-undo')
        assert save(c) == before, label+' Undo changed retained asset bytes'
        ready(c)
        c.click('workshop-redo')
        assert save(c) == after, label+' Redo changed retained asset bytes'
        ready(c)
        receipt['cases'].append(label+' with complete-asset, byte-exact Undo/Redo')
        print('PASS: '+label, flush=True)

    try:
        client = launch('editing')
        client.click('workshop-model-context-skeleton')
        baseline = save(client)
        initial = model(client)
        assert initial['count'] > 30
        assert ': absent' in client.inspect('workshop-model-bone-remove')
        click(client, 'workshop-model-joint-30', 'workshop-model-outliner')
        client.click('workshop-model-rig-pose')
        client.enter('workshop-model-pos-0', '2.125')
        client.click('workshop-model-apply-transform')
        posed = model(client)
        offset, pose = next((offset, row) for offset, row in posed['poses'].items() if row['index'] == 30)
        assert pose['apply'] and pose['local'][0] == 2.125
        client.click('workshop-model-rig-edit')
        click(client, 'workshop-model-joint-0', 'workshop-model-outliner')
        click(client, 'workshop-model-bone-name', 'workshop-model-outliner')
        client.enter('workshop-model-bone-name', 'Workshop extra A')
        click(client, 'workshop-model-bone-add', 'workshop-model-outliner')
        ready(client)
        added = model(client)
        assert added['count'] == initial['count']+1, added
        first = save(client)
        assert first != baseline
        round_trip(client, baseline, first, 'Add child')
        choose(client, 10)
        click(client, 'workshop-model-bone-reparent', 'workshop-model-inspector')
        ready(client)
        reparented = model(client)
        assert reparented['selected'] < added['selected'], (added, reparented)
        assert reparented['poses'][offset]['index'] != 30
        assert reparented['poses'][offset]['local'] == pose['local']
        assert reparented['poses'][offset]['apply'] == pose['apply']
        second = save(client)
        round_trip(client, first, second, 'Reparent extra bone')
        receipt['cases'].append('Temporary pose follows the retained joint through physical reordering')
        click(client, 'workshop-model-bone-name', 'workshop-model-outliner')
        client.enter('workshop-model-bone-name', 'Workshop extra B')
        click(client, 'workshop-model-bone-add', 'workshop-model-outliner')
        ready(client)
        third = save(client)
        child = model(client)
        assert child['count'] == initial['count']+2
        client.click('workshop-mode-animation')
        click(client, 'workshop-joint-'+str(child['selected']), 'workshop-editor-browser')
        assert 'Workshop extra B' in client.inspect('workshop-bone-name')
        client.click('workshop-mode-model')
        # The added child follows its selected parent in depth-first order.
        click(client, 'workshop-model-joint-'+str(reparented['selected']), 'workshop-model-outliner')
        click(client, 'workshop-model-bone-remove', 'workshop-model-inspector')
        ready(client)
        removed = model(client)
        assert removed['count'] == initial['count']+1
        fourth = save(client)
        round_trip(client, third, fourth, 'Remove extra bone and retain child')
        client.click('workshop-mode-animation')
        assert 'Workshop extra B' in client.inspect('workshop-bone-name')
        assert re.search(r'workshop-animation-keys:.*text=\s*$', client.inspect('workshop-animation-keys'), re.M)
        client.click('workshop-mode-model')
        receipt['cases'].append('Explicit unanimated bone selection survives hierarchy editing and Undo/Redo')
        # Paint a visible vertex onto the retained extra bone, then verify the
        # saved envelope independently of the renderer and editor inspector.
        client.click('workshop-model-context-skinning')
        click(client, 'workshop-model-joint-'+str(reparented['selected']), 'workshop-model-outliner')
        for name, value in [('workshop-model-brush-radius', '0.5'), ('workshop-model-brush-strength', '0.25')]:
            click(client, name, 'workshop-model-inspector')
            client.enter(name, value)
        start = client.stderr.stat().st_size
        client.command('ui-inspect', 'workshop-model-surface')
        time.sleep(.3)
        with client.stderr.open(errors='replace') as log:
            log.seek(start)
            surface = log.read()
        point = re.search(r'index=0 visible=1 screen=([\d.e+-]+),([\d.e+-]+)', surface)
        assert point, surface
        text = client.inspect('workshop-model-viewport')
        left, top, width, height = map(float, re.search(r'origin=([\d.-]+),([\d.-]+) size=([\d.-]+),([\d.-]+)', text).groups())
        game_h = min(height, width*.75)
        game_w = game_h*4/3
        client.command('ui-click', round(left+(width-game_w)/2+float(point[1])*game_w),
                       round(top+(height-game_h)/2+float(point[2])*game_h))
        time.sleep(.4)
        ready(client)
        painted = save(client)
        assert painted != fourth
        saved_draft = client.data/'projects/rig-workflow'
        offsets = [int(at, 16) for at in re.findall(r'joint \d+ parent -?\d+ offset 0x([0-9A-Fa-f]+)',
                                                   command('project-model-joints', saved_draft))]
        _, weights, _, _, _ = raw_mesh(saved_draft/'costume.dat', offsets, 0)
        assert abs(weights[0].get(reparented['selected'], 0)-.25) < 2e-6, weights
        round_trip(client, fourth, painted, 'Paint weights onto added bone')
        fourth = painted
        receipt['screenshots'].append(str(client.screenshot()))
        client.click('workshop-mode-action')
        assert 'Down air' in client.inspect('workshop-context-action')
        client.click('workshop-mode-animation')
        client.element('workshop-animation-tracks')
        click(client, 'workshop-joint-'+str(reparented['selected']), 'workshop-editor-browser')
        assert 'Workshop extra B' in client.inspect('workshop-bone-name')
        click(client, 'workshop-bone-px', 'workshop-editor-inspector')
        client.enter('workshop-bone-px', '3.25')
        client.command('ui-key', 'enter')
        click(client, 'workshop-bone-key-0', 'workshop-editor-inspector')
        ready(client)
        keyed = save(client)
        assert keyed['animation.dat'] != fourth['animation.dat']
        action = int(re.search(r'^(\d+) AttackAirLw$', command('project-actions', saved_draft), re.M)[1])
        tracks = command('project-animation', saved_draft, action)
        track = int(re.search(r'^(\d+) joint='+str(reparented['selected'])+r' channel=5 ', tracks, re.M)[1])
        assert '3.25' in command('project-animation-sample', saved_draft, action, track, 0)
        round_trip(client, fourth, keyed, 'Animate added bone with a new Position X channel')
        fourth = keyed
        client.click('workshop-mode-model')
        assert save(client) == fourth
        receipt['cases'].append('Action and Animation views reopen after structural edits')
        shutil.copytree(client.data/'projects/rig-workflow', draft, dirs_exist_ok=True)
        client.stop()
        source.unlink()
        client = launch('reopened')
        reopened = model(client)
        assert reopened['count'] == initial['count']+1
        assert save(client) == fourth
        client.click('workshop-test-character')
        client.element('workshop-return-editor')
        time.sleep(3)
        receipt['screenshots'].append(str(client.screenshot()))
        client.click('workshop-return-editor')
        client.element('workshop-editor-header')
        ready(client)
        client.click('workshop-mode-model')
        assert model(client)['count'] == reopened['count']
        receipt['cases'].append('Saved extra skeleton reopens and enters/leaves real practice')
        receipt['status'] = 'PASS'
    except BaseException as error:
        receipt['status'] = 'FAIL'
        receipt['error'] = repr(error)
        raise
    finally:
        if client:
            client.stop()
        assert source_hashes == {name: hashlib.sha256((args.draft/name).read_bytes()).hexdigest() for name in ASSETS}
        receipt['source_unchanged'] = True
        receipt['exe_sha256'] = hashlib.sha256(args.exe.read_bytes()).hexdigest()
        (output/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print('PASS: native bone controls, structural history, pose identity, reopen and practice')


if __name__ == '__main__':
    main()
