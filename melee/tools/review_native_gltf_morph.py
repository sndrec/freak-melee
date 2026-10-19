"""Import and reimport named GLB morphs in isolated native authoring projects."""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import re
import struct
import subprocess

from review_native_source_skin import Glb, mul, inverse, column_major, hashes


def fixture(path, joints, *, skinned=False, reordered=False, fault=None,
            defaults='node', anonymous=False, mirrored=False, count=4, morph=True):
    g = Glb()
    unit = 1.25 if skinned else 1
    base = [(-3, 1, 0), (2, 1, 0), (2, 6, .25), (-3, 6, -.5)]
    points = [(base[i % 4][0]+(i//4 % 50)*.125, base[i % 4][1]+(i//200)*.125,
               base[i % 4][2]+(i//4)*.00001) for i in range(count)]
    pos = g.accessor([c/unit for p in points for c in p], 5126, count, 'VEC3')
    g.doc['accessors'][pos].update(min=[min(p[c] for p in points)/unit for c in range(3)],
                                   max=[max(p[c] for p in points)/unit for c in range(3)])
    attributes = dict(POSITION=pos, NORMAL=g.accessor([0, 0, 1]*count, 5126, count, 'VEC3'),
                      TEXCOORD_0=g.accessor([0, 0, 1, 0, 1, 1, 0, 1]*(count//4), 5126, count, 'VEC2'))
    targets = {}
    for name in ('Wide', 'Lean', 'Blink'):
        target = {}
        if name != 'Lean':
            values = [c/unit for i in range(count) for c in (.3*(i+1)*4/count, .1*(i % 4), -.05*(i % 4))]
            if fault == 'nan' and name == 'Wide':
                values[0] = float('nan')
            target_count = count-1 if fault == 'bad-count' and name == 'Wide' else count
            target['POSITION'] = g.accessor(values[:target_count*3], 5126, target_count, 'VEC3', sparse=name == 'Wide')
            g.doc['accessors'][target['POSITION']].update(
                min=[min(v for v in values[c:target_count*3:3] if math.isfinite(v)) for c in range(3)],
                max=[max(v for v in values[c:target_count*3:3] if math.isfinite(v)) for c in range(3)])
        if name != 'Blink':
            target['NORMAL'] = g.accessor([c for i in range(count) for c in (.025*(i % 4), -.04*(i % 4+1), .05)],
                                          5126, count, 'VEC3', sparse=name == 'Lean')
        targets[name] = target
    names = ['Lean', 'Wide', 'Blink'] if reordered else ['Wide', 'Lean']
    if fault == 'missing':
        names = ['Wide']
    if fault == 'tangent':
        attributes['TANGENT'] = g.accessor([1, 0, 0, 1]*4, 5126, 4, 'VEC4')
        targets['Wide']['TANGENT'] = g.accessor([.1, 0, 0]*4, 5126, 4, 'VEC3')
    indices = [i+v for i in range(0, count, 4) for v in (0, 1, 2, 0, 2, 3)]
    primitive = dict(attributes=attributes, indices=g.accessor(indices, 5123, len(indices), 'SCALAR'),
                     targets=[targets[name] for name in names])
    mesh = dict(name='Expressions', primitives=[primitive], extras=dict(targetNames=names),
                weights=[dict(Wide=.3, Lean=.45, Blink=.1)[name] for name in names])
    nodes = []
    if skinned:
        shape = [[1.1, .2, 0, 2], [0, .9, -.1, -3], [.1, 0, 1.3, 1.5]]
        source_inverse = [mul(j['inverse'], shape) for j in joints]
        for matrix in source_inverse:
            for row in matrix:
                row[3] /= unit
        targets_by_joint = {j['id']: i for i, j in enumerate(joints)}
        nodes = [dict(name='Armature', children=[])]
        for i, joint in enumerate(joints):
            parent = joint['parent']
            while parent >= 0 and parent not in targets_by_joint:
                parent = joint['parents'][parent]
            local = inverse(source_inverse[i])
            if parent >= 0:
                local = mul(source_inverse[targets_by_joint[parent]], local)
            nodes.append(dict(name=joint['name'], matrix=column_major(local), children=[]))
            nodes[targets_by_joint[parent]+1 if parent >= 0 else 0]['children'].append(i+1)
        binds = g.accessor([v for matrix in source_inverse for v in column_major(matrix)],
                           5126, len(joints), 'MAT4')
        g.doc['skins'] = [dict(joints=list(range(1, len(joints)+1)), inverseBindMatrices=binds, skeleton=0)]
        attributes['JOINTS_0'] = g.accessor([0, 1, 0, 0]*count, 5123, count, 'VEC4')
        attributes['WEIGHTS_0'] = g.accessor([v for i in range(count) for v in (.25+.1*(i % 4), .75-.1*(i % 4), 0, 0)],
                                            5126, count, 'VEC4')
    node = dict(mesh=0, matrix=column_major([[1.2, -.3, .15, 3], [.2, .8, -.1, 2], [0, .1, 1.5, -1]]),
                weights=[dict(Wide=-.25, Lean=1.3, Blink=.2)[name] for name in names])
    if skinned:
        node['skin'] = 0
    if defaults != 'node':
        del node['weights']
    if defaults == 'zero':
        del mesh['weights']
    if anonymous:
        del mesh['extras']
    if mirrored:
        for row in range(3):
            node['matrix'][row] *= -1
    mesh_node = len(nodes)
    if not morph:
        del primitive['targets']
        mesh.pop('weights', None);mesh.pop('extras', None);node.pop('weights', None)
    nodes.append(node)
    g.doc.update(meshes=[mesh], nodes=nodes, scenes=[dict(nodes=[0, mesh_node] if skinned else [0])], scene=0)
    g.save(path)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('tool', 'test', 'draft', 'output'):
        parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    before = hashes(args.draft)
    env = dict(os.environ, LOCALAPPDATA=str(output/'appdata'))
    result = subprocess.run([str(args.tool.resolve()), 'project-model-joints', str(args.draft.resolve())],
        capture_output=True, text=True, encoding='utf-8', env=env, creationflags=subprocess.CREATE_NO_WINDOW)
    assert result.returncode == 0, result.stderr
    rows = re.findall(r'joint (\d+) parent (-?\d+) offset 0x([\da-fA-F]+) flags 0x([\da-fA-F]+) (.+)', result.stdout)
    parents = [int(row[1]) for row in rows]
    raw = (args.draft/'costume.dat').read_bytes()[32:]
    joints = []
    for index, parent, address, flags, name in rows:
        address, flags = int(address, 16), int(flags, 16)
        bind = struct.unpack_from('>I', raw, address+56)[0]
        if not flags & 1 or not bind:
            continue
        values = struct.unpack_from('>12f', raw, bind)
        joints.append(dict(id=int(index), parent=int(parent), parents=parents,
                           name=name.split(' · joint ')[0], inverse=[list(values[r*4:r*4+4]) for r in range(3)]))
    assert len(joints) >= 2
    for name, options in [('original', {}), ('reordered', dict(reordered=True)),
                           ('skin', dict(skinned=True)), ('skin-reordered', dict(skinned=True, reordered=True)),
                           ('mesh-default', dict(defaults='mesh')), ('zero', dict(defaults='zero')),
                           ('anonymous', dict(anonymous=True)), ('mirrored', dict(mirrored=True))]:
        fixture(output/(name+'.glb'), joints, **options)
    fixture(output/'ordinary.glb', joints, morph=False)
    for fault in ('missing', 'bad-count', 'nan', 'tangent'):
        fixture(output/(fault+'.glb'), joints, fault=fault, reordered=fault != 'missing')
    for name, options in [('large', {}), ('large-reordered', dict(reordered=True)),
                          ('large-skin', dict(skinned=True)),
                          ('large-skin-reordered', dict(skinned=True, reordered=True))]:
        fixture(output/(name+'.glb'), joints, count=36000, **options)
    result = subprocess.run([str(args.test.resolve()), str(args.draft.resolve()), str(output)],
        capture_output=True, text=True, encoding='utf-8', env=env, creationflags=subprocess.CREATE_NO_WINDOW)
    receipt = dict(status='PASS' if result.returncode == 0 else 'FAIL', exit_code=result.returncode,
                   output=result.stdout, error=result.stderr, source_unchanged=hashes(args.draft) == before,
                   test_sha256=hashlib.sha256(args.test.read_bytes()).hexdigest(),
                   tool_sha256=hashlib.sha256(args.tool.read_bytes()).hexdigest())
    if not receipt['source_unchanged']:
        receipt['status'] = 'FAIL'
    (output/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(receipt, indent=2))
    assert receipt['status'] == 'PASS'


if __name__ == '__main__':
    main()
