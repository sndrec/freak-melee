"""Paint an editable stock or imported mesh through a hidden, muted Workshop client.

The supplied profile is copied. The chosen vertex's saved weight and complete
Undo/Redo assets are checked; screenshots include the complete Workshop UI.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import time

from review_native_css_room import Client


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'tool', 'control', 'iso', 'profile', 'output'):
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--project', required=True, help='Project directory below Freak Melee')
    parser.add_argument('--mesh', type=int, default=0)
    parser.add_argument('--draft', action='store_true', help='Open the first isolated draft directly')
    parser.add_argument('--reimport', type=Path, help='Reimport this source after painting and verify retained weights and pose')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    receipt = dict(status='RUNNING', mesh=args.mesh, screenshots=[],
                   exe_sha256=hashlib.sha256(args.exe.read_bytes()).hexdigest(),
                   tool_sha256=hashlib.sha256(args.tool.read_bytes()).hexdigest())
    client = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(),
                    output/'client', 'Stock Skin Review', window_size=(1440, 900),
                    control=args.control.resolve())
    try:
        draft = client.data/args.project
        costume = draft/'costume.dat'
        original = costume.read_bytes()
        def tool(*parts):
            result = subprocess.run([str(args.tool.resolve()), *map(str, parts)],
                capture_output=True, text=True, creationflags=subprocess.CREATE_NO_WINDOW)
            assert result.returncode == 0, result.stdout+result.stderr
            return result.stdout
        def ready():
            deadline = time.monotonic()+60
            while time.monotonic() < deadline:
                log = client.folder/'stdout.log'
                start = log.stat().st_size
                client.command('ui-inspect', 'workshop-preview')
                time.sleep(.15)
                with log.open(errors='replace') as file:
                    file.seek(start); text = file.read()
                if 'editing=1 live=0 transitioning=0' in text and '[workshop-camera] active=1' in text:
                    return
                time.sleep(.2)
            raise RuntimeError('Frozen preview did not become ready: '+text)
        def surface():
            start = client.stderr.stat().st_size
            client.command('ui-inspect', 'workshop-model-surface')
            deadline = time.monotonic()+5
            while time.monotonic() < deadline:
                with client.stderr.open(errors='replace') as file:
                    file.seek(start); text = file.read()
                count = re.search(r'ready=1 vertices=(\d+)', text)
                rows = re.findall(r'index=(\d+) visible=1 screen=([\d.e+-]+),([\d.e+-]+)', text)
                all_rows = re.findall(r'\[workshop-model-vertex\]', text)
                if count and len(all_rows) == int(count[1]):
                    return {int(index): (float(x), float(y)) for index, x, y in rows}
                time.sleep(.1)
            raise RuntimeError('Surface query failed: '+text)
        def weights():
            return {int(index): {int(bone): float(weight) for bone, weight in
                    re.findall(r'joint (\d+) ([\d.e+-]+)', tail)}
                    for index, tail in re.findall(r'vertex (\d+):[^\n]*?\)([^\n]*)',
                        tool('project-model-vertices', draft, args.mesh))}
        def save():
            client.click('workshop-save-top')
            assert 'Draft saved.' in client.inspect('workshop-save-status'), 'Draft Save did not complete'
            return costume.read_bytes()
        client.click('nav-5')
        if args.draft:
            client.click('import-fighter'); client.click('workshop-draft-0')
        else:
            client.click('edit-fighter')
        client.element('workshop-editor-header'); ready()
        client.click('workshop-mode-model')
        client.click('workshop-model-context-skinning')
        selector='workshop-model-skin-mesh'
        client.click(selector)
        selected=int(re.search(r'\[ui-inspect-value\] '+selector+r':.* value=(\d+) text=',client.inspect(selector))[1])
        delta=args.mesh-selected
        for _ in range(abs(delta)):client.command('ui-key','down' if delta>0 else 'up')
        client.command('ui-key','enter')
        assert re.search(r'\[ui-inspect-value\] '+selector+r':.* value='+str(args.mesh)+r' text=',client.inspect(selector))
        client.click('workshop-model-bones')
        points = surface()
        assert len(points) > 1, points
        # Pick an isolated projected point so pixel rounding cannot select a
        # different vertex. This exercises the same native mouse brush as users.
        chosen = max(points, key=lambda i: min((points[i][0]-p[0])**2 + (points[i][1]-p[1])**2
                                              for j, p in points.items() if j != i))
        before = weights()
        joints = re.findall(r'joint (\d+) parent -?\d+ offset 0x([\da-fA-F]+) flags 0x([\da-fA-F]+)',
                            tool('project-model-joints', draft))
        data=costume.read_bytes()[32:]
        target = next(int(index) for index, offset, flags in joints
                      if int(flags, 16)&1 and struct.unpack_from('>I',data,int(offset,16)+56)[0] and
                      before[chosen].get(int(index), 0) < .8)
        client.click(f'workshop-model-joint-{target}')
        points = surface()
        receipt.update(vertex=chosen, joint=target, original_sha256=hashlib.sha256(original).hexdigest())
        receipt['screenshots'].append(str(client.screenshot()))
        client.enter('workshop-model-brush-radius', '.05')
        client.enter('workshop-model-brush-strength', '.2')
        text = client.inspect('workshop-model-viewport')
        x, y, width, height = map(float, re.search(r'origin=([\d.-]+),([\d.-]+) size=([\d.-]+),([\d.-]+)', text).groups())
        h = min(height, width*.75); w = h*4/3
        point = points[chosen]
        client.command('ui-click', round(x+(width-w)/2+point[0]*w), round(y+(height-h)/2+point[1]*h))
        time.sleep(.6); ready()
        painted = save()
        assert painted != original, client.inspect('workshop-editor-status')
        after = weights()
        expected = before[chosen].get(target, 0)+.2
        assert abs(after[chosen].get(target, 0)-expected) < 2e-6, (chosen, target, expected, after[chosen])
        retained = surface()
        unchanged = [index for index in points if before[index] == after[index]]
        assert unchanged and all(index in retained for index in unchanged)
        displacement = max(max(abs(points[index][axis]-retained[index][axis]) for axis in range(2))
                           for index in unchanged)
        assert displacement < 1e-5, ('Unpainted stock surface moved', displacement)
        receipt.update(unchanged_vertices=len(unchanged), max_screen_displacement=displacement)
        receipt['screenshots'].append(str(client.screenshot()))
        client.click('workshop-undo'); assert save() == original
        client.click('workshop-redo'); assert save() == painted
        if args.reimport:
            prior_weights=weights();prior_points=surface()
            client.click('workshop-model-context-meshes')
            client.click('workshop-model-reimport')
            client.enter('workshop-model-import-path',str(args.reimport.resolve()))
            client.click('workshop-model-import-load')
            client.click('workshop-model-import-next');client.click('workshop-model-import-next')
            assert re.search(r'\[ui-inspect-value\] workshop-model-import-binding:.* value=keep text=',
                             client.inspect('workshop-model-import-binding'))
            client.click('workshop-model-import-next')
            assert 'visible=0' in client.inspect('workshop-model-import-overlay'),client.inspect('workshop-model-import-note')
            ready();save();assert weights()==prior_weights
            client.click('workshop-model-context-skinning')
            following=surface()
            assert prior_points.keys()==following.keys()
            reimport_displacement=max(max(abs(prior_points[i][a]-following[i][a]) for a in range(2)) for i in prior_points)
            assert reimport_displacement<1e-5, reimport_displacement
            receipt.update(reimport_weights_preserved=True,reimport_screen_displacement=reimport_displacement)
            receipt['screenshots'].append(str(client.screenshot()))
        receipt.update(status='PASS', painted_sha256=hashlib.sha256(painted).hexdigest(),
                       weight_before=before[chosen].get(target, 0), weight_after=after[chosen][target])
        print('PASS: native stock brush, saved weight, exact Undo/Redo')
    except Exception as error:
        receipt.update(status='FAIL', error=str(error))
        raise
    finally:
        (output/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n', encoding='utf-8')
        client.stop()


if __name__ == '__main__':
    main()
