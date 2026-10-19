"""Exercise saved rest edits, Model undo/redo and practice with an isolated draft.

The native model_rest CTest checks geometry independently across rigid, skinned
and morph-target formats and transforms. This review covers real fighter asset
validation and UI paths.
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


def mesh_offsets(asset, joints):
    size=struct.unpack_from('>I',asset,4)[0]
    data=asset[32:32+size]
    word=lambda at:struct.unpack_from('>I',data,at)[0]
    result=set()
    for joint in joints:
        offset=int(joint[2],16)
        if word(offset+4)&(0x20|0x4000):continue
        mesh=word(offset+16)
        while mesh:
            assert mesh not in result
            result.add(mesh)
            mesh=word(mesh+4)
    return result


def make_shape(asset, mesh, mode):
    """Turn only the added fixture quad into a three-target HSD shape mesh."""
    size,relocations=struct.unpack_from('>II',asset,4)
    data=bytearray(asset[32:32+size])
    links=set(struct.unpack_from(f'>{relocations}I',asset,32+size))
    tail=asset[32+size+relocations*4:]
    word=lambda at:struct.unpack_from('>I',data,at)[0]
    half=lambda at:struct.unpack_from('>H',data,at)[0]
    def allocate(count):
        data.extend(b'\0'*(-len(data)%4));at=len(data);data.extend(b'\0'*count);return at
    def link(slot,target):
        struct.pack_into('>I',data,slot,target);links.add(slot)
    polygon=word(mesh+12)
    assert half(polygon+12)&0x3000==0 and not word(polygon+4)
    descriptors=word(polygon+8)
    assert [word(descriptors+i*24) for i in range(3)]==[9,10,255]
    shape=allocate(28)
    struct.pack_into('>HHI',data,shape,2 if mode=='additive' else 1,2 if mode=='additive' else 3,4)
    struct.pack_into('>I',data,shape+16,4)
    for attr in range(2):
        original=descriptors+attr*24
        assert word(original+4)==3 and word(original+12)==4 and half(original+18)==12
        source=word(original+20)
        values=[struct.unpack_from('>3f',data,source+i*12) for i in range(4)]
        descriptor=allocate(48);table=allocate(12);array=allocate(3*4*12)
        data[descriptor:descriptor+24]=data[original:original+24]
        struct.pack_into('>I',data,descriptor+24,255)
        link(descriptor+20,array);link(shape+8+attr*12,descriptor);link(shape+12+attr*12,table)
        for target in range(3):
            indices=allocate(8);link(table+target*4,indices)
            for i,v in enumerate(values):
                if mode=='additive' and target:v=(0,0,.25*target*(i+1))
                elif target:v=(v[0],v[1],v[2]+.25*target*(i+1))
                struct.pack_into('>3f',data,array+(target*4+i)*12,*v)
                struct.pack_into('>H',data,indices+i*2,target*4+i)
    struct.pack_into('>H',data,polygon+12,(half(polygon+12)&~0x3000)|0x1000)
    link(polygon+20,shape)
    result=bytearray(asset[:32])+data+struct.pack(f'>{len(links)}I',*sorted(links))+tail
    struct.pack_into('>III',result,0,len(result),len(data),len(links))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['tool', 'draft', 'output']:
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--exe', type=Path)
    parser.add_argument('--iso', type=Path)
    parser.add_argument('--control', type=Path)
    parser.add_argument('--shape-mode',choices=['average','additive'])
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
    old_meshes=mesh_offsets((project/'costume.dat').read_bytes(),joints) if args.shape_mode else None
    command('project-model-import', project, source, 0, bone, 'add', 1, 'rigid')
    if args.shape_mode:
        asset=(project/'costume.dat').read_bytes()
        added=mesh_offsets(asset,joints)-old_meshes
        assert len(added)==1
        (project/'costume.dat').write_bytes(make_shape(asset,added.pop(),args.shape_mode))
    original = (project/'costume.dat').read_bytes()
    original_names = (project/'workshop_names.txt').read_bytes()
    results = []
    # Each command saves, validates the actual fighter resources and reopens.
    for channel, axis, value in [('rotation',2,.73), ('scale',0,1.7), ('position',0,2.25)]:
        command('project-model-rest', project, bone, channel, axis, value)
        command('project-model-joints', project)
        results.append(dict(case='saved-'+channel, status='PASS'))
    changed = (project/'costume.dat').read_bytes()
    assert changed != original
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
        (destination/'workshop_names.txt').write_bytes(original_names)
        client = Client(args.exe.resolve(), args.iso.resolve(), profile,
            output/'preview', 'Rest Edit Review', window_size=(1440,810),
            control=args.control.resolve() if args.control else None)
        def preview_state():
            log=client.folder/'stdout.log';offset=log.stat().st_size
            client.command('ui-inspect','workshop-preview')
            for _ in range(50):
                time.sleep(.1)
                with log.open(errors='replace') as file:
                    file.seek(offset);text=file.read()
                if '[workshop-member]' in text and '[workshop-camera]' in text:return text
            raise RuntimeError('Preview inspection did not arrive')
        try:
            client.click('nav-5')
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
            state_before=preview_state()
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
            saved_names = saved.with_name('workshop_names.txt')
            edited_names = saved_names.read_bytes()
            assert edited != original
            joint = int(joints[bone][2],16)
            assert abs(struct.unpack_from('>f', edited,32+joint+28)[0] - .5235988) < 1e-5
            client.screenshot()
            client.click('workshop-model-context-meshes')
            surface_after = client.screenshot()
            state_after=preview_state()
            from PIL import Image, ImageChops
            # Exclude labels and toolbar. Both views use the same camera.
            region=(432,235,1156,558)
            before_pixels=Image.open(surface_before).convert('RGB').crop(region)
            after_pixels=Image.open(surface_after).convert('RGB').crop(region)
            difference=ImageChops.difference(before_pixels,after_pixels)
            changed_pixels=sum(any(c>8 for c in pixel) for pixel in difference.getdata())
            (output/'surface-images.json').write_text(json.dumps(dict(before=str(surface_before), after=str(surface_after),changed_pixels=changed_pixels,state_before=state_before,state_after=state_after),indent=2))
            assert changed_pixels < 100, f'Rest geometry changed visibly: {changed_pixels} pixels'
            client.click('workshop-undo')
            client.click('workshop-save-top')
            assert saved.read_bytes() == original, 'Undo did not restore the complete original asset'
            assert saved_names.read_bytes() == original_names, 'Undo did not restore source rest facts'
            client.screenshot()
            client.click('workshop-redo')
            client.click('workshop-save-top')
            assert saved.read_bytes() == edited, 'Redo did not restore the expanded asset'
            assert saved_names.read_bytes() == edited_names, 'Redo did not restore source rest facts'
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
        shape_mode=args.shape_mode,
        tool_sha256=hashlib.sha256(tool.read_bytes()).hexdigest(),
        exe_sha256=hashlib.sha256(args.exe.read_bytes()).hexdigest() if args.exe else None),indent=2)+'\n')
    print('PASS: saved rest edits'+(' and native Model review' if args.exe else ''))


if __name__ == '__main__':
    main()
