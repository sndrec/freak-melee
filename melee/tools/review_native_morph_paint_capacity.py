"""Reject an overflowing morph-normal split without changing the saved draft.

Uses an isolated copy of the unweighted quad from review_native_morph_view.
No game process or network connection is started.
"""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess

from review_native_large_gltf import hashes
from review_native_rest import mesh_offsets


def at_capacity(asset, mesh):
    size, count = struct.unpack_from('>II', asset, 4)
    data = bytearray(asset[32:32+size])
    links = set(struct.unpack_from(f'>{count}I', asset, 32+size))
    tail = asset[32+size+count*4:]
    word = lambda at: struct.unpack_from('>I', data, at)[0]

    def allocate(count):
        data.extend(b'\0'*(-len(data)%4))
        at = len(data)
        data.extend(b'\0'*count)
        return at

    def link(at, target):
        struct.pack_into('>I', data, at, target)
        links.add(at)

    polygon = word(mesh+12)
    shape = word(polygon+20)
    assert struct.unpack_from('>H', data, shape)[0] in (1, 2)
    attrs = word(polygon+8)
    assert [word(attrs+i*24) for i in range(3)] == [9, 10, 255]
    descriptor = word(shape+20)
    assert word(descriptor+4) == 3 and word(descriptor+12) == 4
    old_array = word(descriptor+20)
    old_indices = word(shape+24)
    mode, targets = struct.unpack_from('>HH', data, shape)
    targets += mode == 2
    array = allocate(targets*2000*12)
    table = allocate(targets*4)
    for target in range(targets):
        original_index = struct.unpack_from('>H', data, word(old_indices+target*4))[0]
        vector = bytes(data[old_array+original_index*12:old_array+original_index*12+12])
        indices = allocate(2000*2)
        link(table+target*4, indices)
        for i in range(2000):
            struct.pack_into('>H', data, indices+i*2, target*2000+i)
            data[array+(target*2000+i)*12:array+(target*2000+i+1)*12] = vector
    struct.pack_into('>I', data, shape+16, 2000)
    link(shape+24, table)
    link(descriptor+20, array)
    display = word(polygon+16)
    assert data[display] == 0x90
    corners = struct.unpack_from('>H', data, display+1)[0]
    for i in range(corners):
        struct.pack_into('>H', data, display+3+i*4+2, 0)
    result = bytearray(asset[:32])+data+struct.pack(f'>{len(links)}I', *sorted(links))+tail
    struct.pack_into('>III', result, 0, len(result), len(data), len(links))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('tool', 'draft', 'output'):
        parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    source_hashes = hashes(args.draft)
    draft = output/'draft'
    shutil.copytree(args.draft.resolve(), draft)
    env = dict(os.environ, LOCALAPPDATA=str(output/'appdata'))

    def tool(*parts):
        return subprocess.run([str(args.tool.resolve()), *map(str, parts)],
            capture_output=True, text=True, env=env, creationflags=subprocess.CREATE_NO_WINDOW)

    joints = tool('project-model-joints', draft)
    assert joints.returncode == 0, joints.stderr
    joints = re.findall(r'joint (\d+) parent (-?\d+) offset 0x([\da-fA-F]+)', joints.stdout)
    costume = draft/'costume.dat'
    asset = costume.read_bytes()
    data = asset[32:]
    word = lambda at: struct.unpack_from('>I', data, at)[0]
    meshes = [mesh for mesh in mesh_offsets(asset, joints) if word(mesh+12) and
              struct.unpack_from('>H', data, word(mesh+12)+12)[0] & 0x3000 == 0x1000]
    assert len(meshes) == 1, meshes
    costume.write_bytes(at_capacity(asset, meshes[0]))
    original = hashes(draft)
    vertices = tool('project-model-vertices', draft, 0)
    assert vertices.returncode == 0 and 'vertex 3:' in vertices.stdout, vertices.stdout+vertices.stderr
    result = tool('project-model-paint', draft, 0, 1, -7, 1, 0, .1, .35, 'add')
    receipt = dict(status='PASS' if result.returncode != 0 and hashes(draft) == original else 'FAIL',
                   normals=2000, expected_after_split=2001, exit_code=result.returncode,
                   output=result.stdout, error=result.stderr, draft_unchanged=hashes(draft) == original,
                   source_unchanged=hashes(args.draft) == source_hashes)
    (output/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(receipt, indent=2))
    assert receipt['status'] == 'PASS' and receipt['source_unchanged']


if __name__ == '__main__':
    main()
