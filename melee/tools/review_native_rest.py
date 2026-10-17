"""Exercise saved rest edits, Model undo/redo and practice with an isolated draft.

The native model_rest CTest checks geometry independently across 324 format and
transform cases. This review covers real fighter asset validation and UI paths.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import time

from review_native_skin_weights import make_glb


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['tool', 'draft', 'output']:
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--exe', type=Path)
    parser.add_argument('--iso', type=Path)
    parser.add_argument('--control', type=Path)
    args = parser.parse_args()
    if args.exe and not args.iso:
        parser.error('--exe requires --iso')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    project = output/'draft'
    shutil.copytree(args.draft.resolve(), project)
    tool = args.tool.resolve()
    env = dict(os.environ, LOCALAPPDATA=str(output/'appdata'))

    def command(*parts, success=True):
        result = subprocess.run([str(tool), *map(str, parts)], env=env, cwd=output,
            capture_output=True, text=True, creationflags=subprocess.CREATE_NO_WINDOW)
        with (output/'tool.log').open('a') as log:
            log.write(' '.join(map(str, parts))+'\n'+result.stdout+result.stderr)
        assert (result.returncode == 0) == success, result.stdout+result.stderr
        return result.stdout

    joints = re.findall(r'joint (\d+) parent (-?\d+) offset 0x([0-9A-Fa-f]+)',
                        command('project-model-joints', project))
    assert len(joints) > 5, 'This review uses the supplied Mario draft'
    bone = 5
    source = output/'asymmetric.glb'
    make_glb(source)
    command('project-model-import', project, source, 0, bone, 'add', 1, 'rigid')
    original = (project/'costume.dat').read_bytes()
    results = []
    # Each command saves, validates the actual fighter resources and reopens.
    for channel, axis, value in [('rotation',2,.73), ('scale',0,1.7), ('position',0,2.25)]:
        command('project-model-rest', project, bone, channel, axis, value)
        command('project-model-joints', project)
        results.append(dict(case='saved-'+channel, status='PASS'))
    changed = (project/'costume.dat').read_bytes()
    assert len(changed) > len(original)
    before = hashlib.sha256(changed).hexdigest()
    command('project-model-rest', project, bone, 'scale', 0, 0, success=False)
    assert hashlib.sha256((project/'costume.dat').read_bytes()).hexdigest() == before
    results.append(dict(case='invalid-scale-preserves-saved-asset', status='PASS'))

    if args.exe:
        from review_native_css_room import Client
        profile = output/'preview-profile'
        (profile/'Freak Melee/content').mkdir(parents=True)
        destination = profile/'Freak Melee/projects/rest-review'
        destination.parent.mkdir()
        shutil.copytree(project, destination)
        (destination/'costume.dat').write_bytes(original)
        client = Client(args.exe.resolve(), args.iso.resolve(), profile,
            output/'preview', 'Rest Edit Review', window_size=(1440,810),
            control=args.control.resolve() if args.control else None)
        try:
            client.click('edit-fighter')
            client.element('workshop-editor-header')
            deadline = time.monotonic()+60
            while time.monotonic() < deadline:
                if 'text=Draft preview ready.' in client.inspect('workshop-editor-status'):
                    break
            else:
                raise RuntimeError('Draft preview did not become ready')
            client.click('workshop-mode-model')
            client.click('workshop-model-rig-edit')
            client.click('workshop-model-context-meshes')
            client.click('workshop-model-bones')
            surface_before = client.screenshot()
            client.click('workshop-model-context-skeleton')
            client.click(f'workshop-model-joint-{bone}')
            client.screenshot()
            client.enter('workshop-model-rot-2', '30')
            client.enter('workshop-model-scale-0', '1.6')
            client.enter('workshop-model-pos-0', '2')
            client.click('workshop-model-apply-transform')
            status = client.inspect('workshop-editor-status')
            assert 'Rest rig updated; meshes kept in place' in status, status
            client.click('workshop-save-top')
            saved = client.data/'projects/rest-review/costume.dat'
            edited = saved.read_bytes()
            assert len(edited) > len(original)
            joint = int(joints[bone][2],16)
            assert abs(struct.unpack_from('>f', edited,32+joint+28)[0] - .5235988) < 1e-5
            client.screenshot()
            client.click('workshop-model-context-meshes')
            surface_after = client.screenshot()
            from PIL import Image, ImageChops
            # Exclude labels and toolbar. Both views use the same camera.
            region=(432,235,1156,558)
            before_pixels=Image.open(surface_before).convert('RGB').crop(region)
            after_pixels=Image.open(surface_after).convert('RGB').crop(region)
            difference=ImageChops.difference(before_pixels,after_pixels)
            changed_pixels=sum(any(c>8 for c in pixel) for pixel in difference.getdata())
            (output/'surface-images.json').write_text(json.dumps(dict(before=str(surface_before), after=str(surface_after),changed_pixels=changed_pixels),indent=2))
            assert changed_pixels < 100, f'Rest geometry changed visibly: {changed_pixels} pixels'
            client.click('workshop-undo')
            client.click('workshop-save-top')
            assert saved.read_bytes() == original, 'Undo did not restore the complete original asset'
            client.screenshot()
            client.click('workshop-redo')
            client.click('workshop-save-top')
            assert saved.read_bytes() == edited, 'Redo did not restore the expanded asset'
            for mode in ['pose','animation']:
                client.click('workshop-model-rig-'+mode)
                client.screenshot()
            client.click('workshop-test-character')
            client.element('workshop-return-editor')
            time.sleep(3)
            client.screenshot()
            client.click('workshop-return-editor')
            client.element('workshop-editor-header')
            client.screenshot()
            results.append(dict(case='native-edit-save-undo-redo-modes-and-practice', status='PASS',
                                original_bytes=len(original), edited_bytes=len(edited)))
        finally:
            client.stop()
    (output/'passed.json').write_text(json.dumps(dict(status='PASS', cases=results,
        tool_sha256=hashlib.sha256(tool.read_bytes()).hexdigest(),
        exe_sha256=hashlib.sha256(args.exe.read_bytes()).hexdigest() if args.exe else None),indent=2)+'\n')
    print('PASS: saved rest edits'+(' and native Model review' if args.exe else ''))


if __name__ == '__main__':
    main()
