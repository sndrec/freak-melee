"""Check morph-mesh picking and highlighting in hidden, muted Workshop clients.

Creates isolated average/additive shape fixtures. Copies the source draft,
imports a quad, and keeps that quad as the costume's only rendered mesh.
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

from PIL import Image
from review_native_css_room import Client
from review_native_large_gltf import hashes
from review_native_rest import make_shape, mesh_offsets
from review_native_skin_weights import make_glb
from review_native_morph_skin_fixture import weighted_shape


def only_shape(asset, joints, mesh):
    size, count = struct.unpack_from('>II', asset, 4)
    data = bytearray(asset[32:32+size])
    links = set(struct.unpack_from(f'>{count}I', asset, 32+size))
    tail = asset[32+size+count*4:]
    word = lambda at: struct.unpack_from('>I', data, at)[0]
    for _, _, address in joints:
        address = int(address, 16)
        if word(address+4) & (0x20 | 0x4000):
            continue
        current = word(address+16)
        while current:
            # Keep stock material/texture objects: fighter texture-animation
            # tables still address them even when their geometry is absent.
            if current != mesh:
                struct.pack_into('>I', data, current+12, 0)
                links.discard(current+12)
            current = word(current+4)
    shape = word(word(mesh+12)+20)
    additive = struct.unpack_from('>H', data, shape)[0] & 2
    points = [(-7, 1, 0), (7, 1, 0), (7, 16, 0), (-7, 16, 0)]
    for attr in range(2):
        desc = word(shape+8+attr*12)
        array = word(desc+20)
        for target in range(3):
            for i, point in enumerate(points):
                if attr:
                    point = (0, 0, 0 if target and additive else 1)
                elif target:
                    point = (0, 0, .25*target*(i+1)) if additive else (point[0], point[1], .25*target*(i+1))
                struct.pack_into('>3f', data, array+(target*4+i)*12, *point)
    result = bytearray(asset[:32])+data+struct.pack(f'>{len(links)}I', *sorted(links))+tail
    struct.pack_into('>III', result, 0, len(result), size, len(links))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'tool', 'iso', 'draft', 'output'):
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--weighted', action='store_true')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    source_hashes = hashes(args.draft)
    source = output/'quad.glb'
    make_glb(source)
    receipt = dict(status='RUNNING', weighted=args.weighted, cases=[], exe_sha256=hashlib.sha256(args.exe.read_bytes()).hexdigest())

    def tool(*parts):
        result = subprocess.run([str(args.tool.resolve()), *map(str, parts)],
            capture_output=True, text=True, encoding='utf-8', errors='replace',
            env=dict(os.environ, LOCALAPPDATA=str(output/'tool-profile')),
            creationflags=subprocess.CREATE_NO_WINDOW)
        assert result.returncode == 0, result.stdout+result.stderr
        return result.stdout

    try:
        for mode in ('average', 'additive'):
            folder = output/mode
            draft = folder/'profile/Freak Melee/projects/morph-review'
            shutil.copytree(args.draft.resolve(), draft)
            (folder/'profile/Freak Melee/content').mkdir()
            joints = re.findall(r'joint (\d+) parent (-?\d+) offset 0x([0-9A-Fa-f]+)', tool('project-model-joints', draft))
            previous = mesh_offsets((draft/'costume.dat').read_bytes(), joints)
            tool('project-model-import', draft, source, 0, 0, 'add', 1, 'rigid')
            asset = (draft/'costume.dat').read_bytes()
            added = mesh_offsets(asset, joints)-previous
            assert len(added) == 1
            mesh = added.pop()
            asset = only_shape(make_shape(asset, mesh, mode), joints, mesh)
            if args.weighted:
                asset = weighted_shape(asset, joints, mesh)
            (draft/'costume.dat').write_bytes(asset)
            client = None
            try:
                client = Client(args.exe.resolve(), args.iso.resolve(), folder/'profile',
                                folder/'client', 'Morph review', window_size=(1440, 900))
                client.click('nav-5'); client.click('import-fighter'); client.click('workshop-draft-0')
                client.element('workshop-editor-header')
                deadline = time.monotonic()+60
                while time.monotonic() < deadline:
                    if 'text=Draft preview ready.' in client.inspect('workshop-editor-status'):
                        break
                    time.sleep(.15)
                else:
                    raise RuntimeError('Preview did not open')
                client.click('workshop-mode-model'); client.click('workshop-model-context-meshes')
                viewport = client.inspect('workshop-model-viewport')
                x, y, width, height = map(float, re.search(r'origin=([\d.-]+),([\d.-]+) size=([\d.-]+),([\d.-]+)', viewport).groups())
                region = (int(x), int(y), int(x+width), int(y+height))

                def capture():
                    path = client.screenshot()
                    pixels = Image.open(path).convert('RGB').crop(region)
                    blue = sum(abs(r-137)<8 and abs(g-210)<8 and abs(b-255)<8 for r,g,b in pixels.getdata())
                    return dict(path=str(path), blue_pixels=blue)

                client.command('ui-move', 30, 30); before = capture()
                client.command('ui-move', *client.element('workshop-model-mesh-0')); tree = capture()
                assert tree['blue_pixels'] > before['blue_pixels']+1000, (before, tree)
                if args.weighted:
                    drawn = Image.open(before['path']).convert('RGB').crop(region)
                    highlight = Image.open(tree['path']).convert('RGB').crop(region)
                    surface = matched = highlighted = 0
                    for a,b in zip(drawn.getdata(), highlight.getdata()):
                        white = all(c > 240 for c in a)
                        blue = all(abs(c-target)<8 for c,target in zip(b,(137,210,255)))
                        surface += white
                        highlighted += blue
                        matched += white and blue
                    assert surface > 1000 and matched/surface > .95 and matched/highlighted > .95
                    tree['drawn_surface_pixels'] = surface
                    tree['overlapping_surface_pixels'] = matched
                # Pick an interior surface pixel away from the bone handles at
                # the viewport center; those handles deliberately receive input.
                image = Image.open(tree['path']).convert('RGB')
                points = [(px,py) for py in range(region[1],region[3]) for px in range(region[0],region[2])
                          if all(abs(a-b)<8 for a,b in zip(image.getpixel((px,py)),(137,210,255)))]
                left,right=min(px for px,py in points),max(px for px,py in points)
                top,bottom=min(py for px,py in points),max(py for px,py in points)
                target=(left+(right-left)*.25,top+(bottom-top)*.25)
                point=min(points,key=lambda p:(p[0]-target[0])**2+(p[1]-target[1])**2)
                client.command('ui-move', 30, 30)
                client.command('ui-move', *point); picked = capture()
                assert picked['blue_pixels'] > before['blue_pixels']+1000, (before, picked)
                receipt['cases'].append(dict(mode=mode, status='PASS', before=before, outliner_hover=tree, viewport_hover=picked))
            finally:
                if client:
                    client.stop()
        receipt['status'] = 'PASS'
    except Exception as error:
        receipt.update(status='FAIL', error=str(error))
    finally:
        receipt['source_unchanged'] = hashes(args.draft) == source_hashes
        if not receipt['source_unchanged']:
            receipt.update(status='FAIL', error='The source draft changed')
        (output/'receipt.json').write_text(json.dumps(receipt, indent=2), encoding='utf-8')
    print(json.dumps(receipt, indent=2))
    assert receipt['status'] == 'PASS', receipt


if __name__ == '__main__':
    main()
