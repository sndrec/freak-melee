"""Import large GLBs through native construction, painting and saved reimport.

Uses copied projects and hidden command-line processes. The source project is
hashed before and after the review. No game window or audio is started.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess

from review_native_source_skin import Glb, inverse, mul, column_major

VERTICES = 70002


def fixture(path, joints, *, skinned=False, gradient=False, deformed=False, bad=False):
    g = Glb()
    points = [(i // 3 % 157 * .02 + (.015 if i % 3 == 1 else 0),
               i // 3 // 157 * .02 + (.015 if i % 3 == 2 else 0),
               i // 3 % 7 * .001) for i in range(VERTICES)]
    points[-1] = points[-2]
    if deformed:
        points = [(x, y, z + .25) for x, y, z in points]
    pos = g.accessor([v for p in points for v in p], 5126, VERTICES, 'VEC3')
    g.doc['accessors'][pos].update(min=[min(p[c] for p in points) for c in range(3)],
                                   max=[max(p[c] for p in points) for c in range(3)])
    normal = g.accessor([0, 0, 1] * VERTICES, 5126, VERTICES, 'VEC3')
    uv = g.accessor([v for i in range(VERTICES) for v in (i % 17 / 16, i % 23 / 22)],
                    5126, VERTICES, 'VEC2')
    indices = list(range(VERTICES)) + [0, 65536, VERTICES-1]
    index_view = g.view(struct.pack('<'+'I'*len(indices), *indices))
    g.doc['accessors'].append(dict(bufferView=index_view, componentType=5125,
                                   count=len(indices), type='SCALAR'))
    primitive = dict(attributes=dict(POSITION=pos, NORMAL=normal, TEXCOORD_0=uv),
                     indices=len(g.doc['accessors'])-1)
    if bad:
        del primitive['attributes']['NORMAL']
    nodes = [dict(name='Armature', children=[])]
    if skinned:
        targets = {joint['id']: i for i, joint in enumerate(joints)}
        for joint in joints:
            parent = joint['parent']
            while parent >= 0 and parent not in targets:
                parent = joint['parents'][parent]
            bind = inverse(joint['inverse'])
            local = mul(joints[targets[parent]]['inverse'], bind) if parent >= 0 else bind
            nodes.append(dict(name=joint['name'], matrix=column_major(local), children=[]))
        for at, joint in enumerate(joints):
            parent = joint['parent']
            while parent >= 0 and parent not in targets:
                parent = joint['parents'][parent]
            nodes[targets[parent]+1 if parent >= 0 else 0]['children'].append(at+1)
        binds = g.accessor([v for joint in joints for v in column_major(joint['inverse'])],
                           5126, len(joints), 'MAT4')
        g.doc['skins'] = [dict(joints=list(range(1, len(joints)+1)), inverseBindMatrices=binds, skeleton=0)]
        primitive['attributes']['JOINTS_0'] = g.accessor([0, 1, 0, 0]*VERTICES, 5123, VERTICES, 'VEC4')
        weights = []
        for i in range(VERTICES):
            weight = (i+1)/(VERTICES+1) if gradient else .25
            weights.extend((weight, 1-weight, 0, 0))
        primitive['attributes']['WEIGHTS_0'] = g.accessor(weights, 5126, VERTICES, 'VEC4')
    mesh_node = len(nodes)
    nodes.append(dict(name='Lattice mesh', mesh=0, **({'skin': 0} if skinned else {})))
    g.doc.update(meshes=[dict(name='Lattice', primitives=[primitive])], nodes=nodes,
                 scenes=[dict(nodes=[0, mesh_node])], scene=0)
    g.save(path)


def hashes(folder):
    return {p.relative_to(folder).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in folder.rglob('*') if p.is_file()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('tool', 'test', 'draft', 'output'):
        parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    draft = args.draft.resolve()
    before = hashes(draft)
    env = dict(os.environ, LOCALAPPDATA=str(output/'appdata'))
    result = subprocess.run([str(args.tool.resolve()), 'project-model-joints', str(draft)],
        env=env, capture_output=True, text=True, encoding='utf-8', creationflags=subprocess.CREATE_NO_WINDOW)
    assert result.returncode == 0, result.stderr
    matches = re.findall(r'joint (\d+) parent (-?\d+) offset 0x([0-9A-Fa-f]+) flags 0x([0-9A-Fa-f]+) (.+)', result.stdout)
    parents = [int(row[1]) for row in matches]
    raw = (draft/'costume.dat').read_bytes()[32:]
    joints = []
    for at, parent, offset, flags, name in matches:
        offset, flags = int(offset, 16), int(flags, 16)
        ptr = struct.unpack_from('>I', raw, offset+56)[0]
        if not flags & 1 or not ptr:
            continue
        values = struct.unpack_from('>12f', raw, ptr)
        joints.append(dict(id=int(at), parent=int(parent), parents=parents,
                           name=name.split(' · joint ')[0],
                           inverse=[list(values[r*4:r*4+4]) for r in range(3)]))
    assert len(joints) > 2
    for name, options in [('rigid', {}), ('auto', {}), ('source', dict(skinned=True)),
                          ('gradient', dict(skinned=True, gradient=True)),
                          ('deformed', dict(deformed=True)), ('bad', dict(bad=True))]:
        fixture(output/(name+'.glb'), joints, **options)
    result = subprocess.run([str(args.test.resolve()), str(draft), str(output)],
        cwd=output, env=env, capture_output=True, text=True, encoding='utf-8', errors='replace',
        creationflags=subprocess.CREATE_NO_WINDOW)
    (output/'native.log').write_text(result.stdout+result.stderr, encoding='utf-8')
    unchanged = hashes(draft) == before
    receipt = dict(status='PASS' if result.returncode == 0 and unchanged else 'FAIL',
        source_unchanged=unchanged, vertices=VERTICES, corners=VERTICES+3,
        test_sha256=hashlib.sha256(args.test.read_bytes()).hexdigest(),
        tool_sha256=hashlib.sha256(args.tool.read_bytes()).hexdigest(), output=result.stdout,
        error=result.stderr, exit_code=result.returncode)
    if result.returncode == 0:
        receipt['checks'] = int(re.search(r'PASS: (\d+)', result.stdout)[1])
    (output/'receipt.json').write_text(json.dumps(receipt, indent=2), encoding='utf-8')
    print(result.stdout+result.stderr)
    assert receipt['status'] == 'PASS', receipt


if __name__ == '__main__':
    main()
