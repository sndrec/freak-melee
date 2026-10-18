"""Exercise imported skinning, temporary poses, clip painting and native save/undo.

Uses a copied Mario draft and hidden/muted clients. Surface coordinates come from
the actual renderer's vertex query; weight checks read the saved DAT independently.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time

from review_native_css_room import Client
from review_native_skin_weights import make_glb, raw_mesh


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'tool', 'control', 'iso', 'draft', 'output']:
        parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    profile = output/'profile'
    draft = profile/'Freak Melee/projects/model-workflow'
    shutil.copytree(args.draft.resolve(), draft)
    (profile/'Freak Melee/content').mkdir()
    source = output/'asymmetric.glb'
    make_glb(source)
    env = dict(os.environ, LOCALAPPDATA=str(output/'tool-profile'))

    def command(*parts):
        result = subprocess.run([str(args.tool.resolve()), *map(str, parts)],
            env=env, cwd=output, capture_output=True, text=True,
            creationflags=subprocess.CREATE_NO_WINDOW)
        with (output/'tool.log').open('a') as log:
            log.write(' '.join(map(str, parts))+'\n'+result.stdout+result.stderr)
        assert result.returncode == 0, result.stdout+result.stderr
        return result.stdout

    joints = re.findall(r'joint (\d+) parent (-?\d+) offset 0x([0-9A-Fa-f]+)',
                         command('project-model-joints', draft))
    offsets = [int(j[2], 16) for j in joints]
    command('project-model-import', draft, source, 0, 0, 'add', 1, 'auto')
    mesh, initial, positions, normals, _ = raw_mesh(draft/'costume.dat', offsets)
    assert mesh == 0, 'The added root-owned fixture should be first in this draft'
    receipt = dict(status='RUNNING', mesh=mesh, cases=[], screenshots=[], failures=[])

    def launch(folder):
        client = Client(args.exe.resolve(), args.iso.resolve(), profile,
            output/folder, 'Model Workflow Review', window_size=(1440, 810),
            control=args.control.resolve())
        client.click('nav-5')
        client.click('edit-fighter')
        client.element('workshop-editor-header')
        ready(client)
        client.click('workshop-mode-model')
        return client

    def ready(client):
        deadline = time.monotonic()+60
        while time.monotonic() < deadline:
            try:
                state = preview_state(client)
            except RuntimeError:
                continue
            if 'editing=1 live=0 transitioning=0' in state and '[workshop-camera] active=1' in state:
                return
        raise RuntimeError('Draft preview did not become ready')

    def surface(client):
        start = client.stderr.stat().st_size if client.stderr.exists() else 0
        client.command('ui-inspect', 'workshop-model-surface')
        for _ in range(50):
            time.sleep(.1)
            with client.stderr.open(errors='replace') as log:
                log.seek(start)
                text = log.read()
            rows = re.findall(r'index=(\d+) visible=(\d+) screen=([\d.e+-]+),([\d.e+-]+) bind=([\d.e+-]+),([\d.e+-]+),([\d.e+-]+)', text)
            if len(rows) == 4:
                return {int(r[0]): dict(visible=int(r[1]), screen=tuple(map(float, r[2:4])), bind=tuple(map(float, r[4:7]))) for r in rows}, text
        raise RuntimeError('Surface query failed: '+text)

    def preview_state(client):
        log = client.folder/'stdout.log'
        start = log.stat().st_size
        client.command('ui-inspect', 'workshop-preview')
        for _ in range(50):
            time.sleep(.1)
            with log.open(errors='replace') as file:
                file.seek(start)
                text = file.read()
            if '[workshop-camera]' in text and '[workshop-member]' in text:
                return text
        raise RuntimeError('Preview state did not arrive')

    def pixel(client, point):
        text = client.inspect('workshop-model-viewport')
        left, top, width, height = map(float, re.search(
            r'origin=([\d.-]+),([\d.-]+) size=([\d.-]+),([\d.-]+)', text).groups())
        h = min(height, width*.75)
        w = h*4/3
        return round(left+(width-w)/2+point[0]*w), round(top+(height-h)/2+point[1]*h)

    def capture(client):
        receipt['screenshots'].append(str(client.screenshot()))

    def save(client):
        client.click('workshop-save-top')
        return (client.data/'projects/model-workflow/costume.dat').read_bytes()

    client = launch('editing')
    try:
        saved = client.data/'projects/model-workflow/costume.dat'
        original = saved.read_bytes()
        animation = client.data/'projects/model-workflow/animation.dat'
        animation_before = animation.read_bytes()
        client.click('workshop-model-context-skinning')
        client.click('workshop-model-bones')
        rest, _ = surface(client)
        client.click('workshop-model-context-skeleton')
        client.click('workshop-model-rig-pose')
        client.click('workshop-model-joint-1')
        client.enter('workshop-model-pos-0', '6')
        client.click('workshop-model-apply-transform')
        client.click('workshop-model-context-skinning')
        client.click('workshop-model-joint-2')
        posed, report = surface(client)
        posed_report = report
        posed_state = preview_state(client)
        assert abs(posed[0]['screen'][0]-rest[0]['screen'][0]) > .08, report
        assert save(client) == original, 'A temporary pose changed the costume'
        assert animation.read_bytes() == animation_before, 'A temporary pose inserted animation keys'
        capture(client)
        client.enter('workshop-model-brush-radius', '0.5')
        client.enter('workshop-model-brush-strength', '0.25')
        assert posed[0]['visible']
        client.command('ui-click', *pixel(client, posed[0]['screen']))
        time.sleep(.6)
        status = client.inspect('workshop-editor-status')
        assert 'vertices painted' in status or 'Draft preview ready.' in status, status
        ready(client)
        retained, report = surface(client)
        receipt['pose_refresh'] = dict(before=posed, after=retained,
            report_before=posed_report, report_after=report,
            state_before=posed_state, state_after=preview_state(client))
        (output/'pose-refresh.json').write_text(json.dumps(receipt['pose_refresh'], indent=2))
        if abs(retained[1]['screen'][0]-posed[1]['screen'][0]) >= 1e-6:
            receipt['failures'].append('Untouched vertex shifted after pose painting')
        painted = save(client)
        assert painted != original
        _, weights, p, n, _ = raw_mesh(saved, offsets, mesh)
        assert p == positions and n == normals
        assert abs(weights[0].get(2, 0)-.25) < 2e-6, weights
        for i in [1, 2, 3]:
            assert weights[i].keys() == initial[i].keys()
            assert all(abs(weights[i][j]-initial[i][j]) < 2e-7 for j in initial[i]), (i, weights[i], initial[i])
        capture(client)
        client.click('workshop-undo')
        assert save(client) == original
        client.click('workshop-redo')
        assert save(client) == painted
        receipt['cases'].append('posed-surface stroke changes only the chosen bind-space vertex; byte-exact Undo/Redo')

        client.click('workshop-model-rig-animation')
        client.click('workshop-model-frame')
        client.command('ui-key', 'home')
        for _ in range(10):
            client.command('ui-key', 'right')
        time.sleep(.3)
        animated, report = surface(client)
        assert 'mode=2' in report and 'frame=10' in report, report
        client.click('workshop-model-rig-pose')
        retained, report = surface(client)
        if abs(retained[1]['screen'][0]-posed[1]['screen'][0]) >= 1e-6:
            receipt['failures'].append('Pose switch did not restore exact vertex projection')
        client.click('workshop-model-rig-animation')
        client.enter('workshop-model-brush-strength', '0.2')
        client.click('workshop-model-play')
        # Start on empty space to verify auto-pause without racing the moving
        # surface between the diagnostic query and the next input message.
        client.command('ui-click', *pixel(client, (.95, .1)))
        time.sleep(.2)
        animated, report = surface(client)
        assert 'playing=0' in report, report
        assert save(client) == painted, 'An empty-space stroke changed the costume'
        assert animated[2]['visible']
        client.command('ui-click', *pixel(client, animated[2]['screen']))
        time.sleep(.6)
        status = client.inspect('workshop-editor-status')
        assert 'vertices painted' in status or 'Draft preview ready.' in status, status
        ready(client)
        _, report = surface(client)
        assert 'playing=0' in report, report
        final = save(client)
        _, weights, p, n, _ = raw_mesh(saved, offsets, mesh)
        assert p == positions and n == normals and final != painted
        assert abs(weights[2].get(2, 0)-.2) < 2e-6, weights
        assert animation.read_bytes() == animation_before
        capture(client)
        receipt['cases'].append('paused animation stroke preserves geometry and animation bytes')

        client.click('workshop-model-rig-edit')
        client.click('workshop-model-context-skeleton')
        client.click('workshop-model-joint-2')
        before = save(client)
        client.enter('workshop-model-rot-2', '20')
        client.click('workshop-model-apply-transform')
        assert 'Rest rig updated' in client.inspect('workshop-editor-status')
        ready(client)
        after = save(client)
        assert after != before
        client.click('workshop-undo')
        assert save(client) == before
        ready(client)
        receipt['cases'].append('rest edit compensates painted mesh and Undo restores exact data')
        capture(client)
        shutil.copytree(client.data/'projects/model-workflow', draft, dirs_exist_ok=True)
        receipt['final_costume_sha256'] = hashlib.sha256(final).hexdigest()
    finally:
        client.stop()

    source.unlink()
    client = launch('reopened')
    try:
        assert (client.data/'projects/model-workflow/costume.dat').read_bytes() == final
        client.click('workshop-model-context-skinning')
        client.click('workshop-model-joint-2')
        capture(client)
        client.click('workshop-test-character')
        client.element('workshop-return-editor')
        time.sleep(3)
        capture(client)
        client.click('workshop-return-editor')
        client.element('workshop-editor-header')
        capture(client)
        receipt['cases'].append('relaunch without GLB, Test Character and return retain the saved mesh')
    finally:
        client.stop()
    command('source-check', draft, draft/'behavior.fms')
    receipt['status'] = 'FAIL' if receipt['failures'] else 'PASS'
    receipt['exe_sha256'] = hashlib.sha256(args.exe.read_bytes()).hexdigest()
    (output/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
    assert not receipt['failures'], receipt['failures']
    print('PASS: native posed/animated painting, rest edits, save/undo, relaunch and practice')


if __name__ == '__main__':
    main()
