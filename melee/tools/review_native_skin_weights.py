"""Exercise imported mesh skin edits through the native tool and raw DAT records.

Copies a supplied draft into a fresh output directory. The original stays intact.
All tool calls reopen saved assets; the independent DAT reader checks actual
envelope records rather than relying solely on the tool's own weight decoder.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import time


POSITIONS = [(-12, 5, 5.25), (-9.6, 7, 4), (-8, 11, 5.75), (-11, 9, 7)]


def make_glb(path):
    values = [x for p in POSITIONS for x in p]
    binary = struct.pack('<12f', *values) + struct.pack('<12f', *([0, 0, 1]*4))
    binary += struct.pack('<6H', 0, 1, 2, 0, 2, 3)
    document = dict(asset=dict(version='2.0'), buffers=[dict(byteLength=len(binary))],
        bufferViews=[dict(buffer=0, byteOffset=0, byteLength=48),
                     dict(buffer=0, byteOffset=48, byteLength=48),
                     dict(buffer=0, byteOffset=96, byteLength=12)],
        accessors=[dict(bufferView=0, componentType=5126, count=4, type='VEC3',
                        min=[-12,5,4], max=[-8,11,7]),
                   dict(bufferView=1, componentType=5126, count=4, type='VEC3'),
                   dict(bufferView=2, componentType=5123, count=6, type='SCALAR')],
        meshes=[dict(name='Weight precision fixture', primitives=[dict(
            attributes=dict(POSITION=0, NORMAL=1), indices=2, mode=4)])],
        nodes=[dict(mesh=0)], scenes=[dict(nodes=[0])], scene=0)
    text = json.dumps(document).encode()
    text += b' '*(-len(text)%4)
    binary += b'\0'*(-len(binary)%4)
    path.write_bytes(struct.pack('<III', 0x46546c67, 2, 28+len(text)+len(binary))+
                     struct.pack('<II', len(text), 0x4e4f534a)+text+
                     struct.pack('<II', len(binary), 0x004e4942)+binary)


def raw_mesh(path, joint_offsets, wanted=None):
    full = path.read_bytes()
    size = struct.unpack_from('>I', full, 4)[0]
    data = full[32:32+size]
    word = lambda at: struct.unpack_from('>I', data, at)[0]
    half = lambda at: struct.unpack_from('>H', data, at)[0]
    meshes = []
    for joint in joint_offsets:
        if word(joint+4)&(0x20|0x4000):
            continue
        mesh = word(joint+16)
        while mesh:
            meshes.append(mesh)
            mesh = word(mesh+4)
    for ordinal, mesh in enumerate(meshes):
        if wanted is not None and ordinal != wanted:
            continue
        polygon = word(mesh+12)
        vertices = {}
        raw_entries = {}
        positions_bytes = None
        normals_bytes = None
        matches = False
        while polygon:
            if half(polygon+12)&0x3000 != 0x2000:
                break
            descriptor = word(polygon+8)
            # The import writer's PNMTXIDX / indexed float XYZ / normal layout.
            if [word(descriptor+i*24) for i in range(3)] != [0,9,10]:
                break
            if word(descriptor+36) != 4 or half(descriptor+42) != 12:
                break
            positions = word(descriptor+44)
            actual = [struct.unpack_from('>3f', data, positions+i*12) for i in range(4)]
            if any(abs(a-b)>1e-6 for p,q in zip(actual,POSITIONS) for a,b in zip(p,q)):
                break
            positions_bytes = data[positions:positions+48]
            normals = word(descriptor+68)
            normals_bytes = data[normals:normals+48]
            display, table = word(polygon+16), word(polygon+20)
            assert data[display] == 0x90
            stride = 7 if word(descriptor+72) == 13 else 5
            for i in range(half(display+1)):
                entry = display+3+i*stride
                slot, vertex = data[entry]//3, half(entry+1)
                assert data[entry]%3 == 0
                influence = word(table+slot*4)
                weights, entries = {}, []
                while word(influence):
                    bone_offset = word(influence)
                    bone = joint_offsets.index(bone_offset)
                    weight = struct.unpack_from('>f',data,influence+4)[0]
                    assert math.isfinite(weight) and weight>0
                    entries.append((bone,weight))
                    weights[bone] = weights.get(bone,0)+weight
                    influence += 8
                assert abs(sum(weights.values())-1)<2e-6
                assert entries[0][1] < 1-2**-23, 'HSD would skip inverse binds'
                if vertex in vertices:
                    assert vertices[vertex] == weights
                vertices[vertex], raw_entries[vertex] = weights, entries
            matches = True
            polygon = word(polygon+4)
        if matches and not polygon:
            assert set(vertices) == set(range(4))
            return ordinal, vertices, positions_bytes, normals_bytes, raw_entries
    raise AssertionError('The imported asymmetric four-vertex mesh was not found')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['tool','draft','output']:
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--exe',type=Path,help='Also open Model modes and Test Character')
    parser.add_argument('--iso',type=Path)
    parser.add_argument('--control',type=Path)
    args = parser.parse_args()
    if args.exe and not args.iso:
        parser.error('--exe requires --iso')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    project = output/'draft'
    shutil.copytree(args.draft.resolve(), project)
    tool = args.tool.resolve()
    env = dict(os.environ, LOCALAPPDATA=str(output/'appdata'))

    def command(*args, success=True):
        result = subprocess.run([str(tool), *map(str,args)], env=env, cwd=output,
            capture_output=True, text=True, creationflags=subprocess.CREATE_NO_WINDOW)
        with (output/'tool.log').open('a') as log:
            log.write(' '.join(map(str,args))+'\n'+result.stdout+result.stderr)
        assert (result.returncode == 0) == success, result.stdout+result.stderr
        return result.stdout

    joints = re.findall(r'joint (\d+) parent (-?\d+) offset 0x([0-9A-Fa-f]+) flags 0x([0-9A-Fa-f]+)',
                         command('project-model-joints',project))
    offsets = [int(j[2],16) for j in joints]
    costume_data = (project/'costume.dat').read_bytes()[32:]
    deform = [int(j[0]) for j in joints if int(j[3],16)&1 and
              struct.unpack_from('>I',costume_data,int(j[2],16)+56)[0]]
    source = output/'asymmetric.glb'
    make_glb(source)
    command('project-model-import',project,source,0,0,'add',1,'auto')
    costume = project/'costume.dat'
    mesh, current, positions, normals, _ = raw_mesh(costume,offsets)
    initial = current
    used = {j for weights in current.values() for j in weights}
    targets = [j for j in deform if j not in used][:2]
    assert len(targets)==2
    for target in targets:
        before = current
        command('project-model-paint',project,mesh,target,0,8,0,100,.173245,'add')
        _,current,p,n,_ = raw_mesh(costume,offsets,mesh)
        assert p==positions and n==normals
        for vertex,weights in current.items():
            amount = .173245*(1-math.dist(POSITIONS[vertex],(0,8,0))/100)
            assert abs(weights[target]-amount)<2e-6
            for joint,weight in before[vertex].items():
                assert abs(weights[joint]-weight*(1-amount))<2e-6
            assert len(weights)==len(before[vertex])+1
    assert all(len(w)>=3 for w in current.values())
    assert any(abs(weight*32-round(weight*32))>.001 for weights in current.values() for weight in weights.values())
    before = current
    command('project-model-import',project,source,0,0,mesh,1,'transfer')
    _,current,p,n,_ = raw_mesh(costume,offsets,mesh)
    assert p==positions and n==normals
    for vertex,weights in current.items():
        assert weights.keys()==before[vertex].keys()
        assert all(abs(weight-before[vertex][joint])<2e-7 for joint,weight in weights.items())
    source.unlink()  # Subsequent saved edits use embedded data alone.
    command('project-model-vertices',project,mesh)
    command('project-model-paint',project,mesh,targets[0],*POSITIONS[0],.1,1,'subtract')
    _,current,_,_,_ = raw_mesh(costume,offsets,mesh)
    assert targets[0] not in current[0]
    command('project-model-paint',project,mesh,targets[0],*POSITIONS[0],.1,.99999994,'add')
    _,current,p,n,entries = raw_mesh(costume,offsets,mesh)
    assert p==positions and n==normals
    assert len(current[0])>1 and current[0][targets[0]]>1-2**-23
    command('project-model-paint',project,mesh,targets[0],*POSITIONS[0],.1,1,'add')
    _,current,p,n,entries = raw_mesh(costume,offsets,mesh)
    assert p==positions and n==normals
    assert current[0]=={targets[0]:1}
    assert entries[0]==[(targets[0],.5),(targets[0],.5)]
    for bone in [bone for bone in deform if bone != targets[0]][:15]:
        command('project-model-paint',project,mesh,bone,*POSITIONS[0],.1,.035,'add')
    _,current,p,n,_ = raw_mesh(costume,offsets,mesh)
    assert p==positions and n==normals and len(current[0])==16
    extra = next(bone for bone in deform if bone not in current[0])
    saved = {p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in project.iterdir() if p.is_file()}
    command('project-model-paint',project,mesh,extra,*POSITIONS[0],.1,.035,'add',success=False)
    assert saved=={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in project.iterdir() if p.is_file()}
    command('source-check',project,project/'behavior.fms')
    if args.exe:
        from review_native_css_room import Client
        profile = output/'preview-profile'
        (profile/'Freak Melee/content').mkdir(parents=True)
        destination = profile/'Freak Melee/projects/weight-review'
        destination.parent.mkdir()
        shutil.copytree(project,destination)
        os.environ['FREAK_MELEE_CHAT_ENDPOINT']='http://127.0.0.1:1'
        client = Client(args.exe.resolve(),args.iso.resolve(),profile,output/'preview',
                        'Skin Weight Review',window_size=(1440,810),
                        control=args.control.resolve() if args.control else None)
        try:
            client.click('edit-fighter')
            client.element('workshop-editor-header')
            deadline=time.monotonic()+60
            while time.monotonic()<deadline:
                if 'text=Draft preview ready.' in client.inspect('workshop-editor-status'):
                    break
            else:
                raise RuntimeError('Weighted draft did not reach its frozen preview')
            client.click('workshop-mode-model')
            client.click('workshop-model-context-skinning')
            bone = max(current[0],key=current[0].get)
            client.click(f'workshop-model-joint-{bone}')
            client.screenshot()
            client.click('workshop-model-rig-pose')
            client.screenshot()
            client.click('workshop-model-rig-animation')
            client.screenshot()
            client.click('workshop-test-character')
            client.element('workshop-return-editor')
            time.sleep(3)
            client.screenshot()
            client.click('workshop-return-editor')
            client.element('workshop-editor-header')
            client.screenshot()
        finally:
            client.stop()
    (output/'passed.json').write_text(json.dumps(dict(status='PASS',mesh=mesh,
        initial_weights=initial,painted_weights=before,final_weights=current,
        checks=['floating weights and additional influences in actual saved DAT',
                'other influence proportions preserved', 'positions and normals unchanged',
                'replacement transfer retains all influences', 'single-bone inverse-bind envelope path',
                'sixteen saved influences and transactional capacity rejection',
                'source and assets validate without the glTF file'],
                live_preview=bool(args.exe)),indent=2)+'\n')
    print('PASS: saved multi-bone weights, precision, transfer, single-bone envelopes and source validation')


if __name__=='__main__':
    main()
