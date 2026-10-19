"""Compare independent reflected texture attachments in the native Workshop.

Creates a fresh Mario draft, imports a sphere, and gives its reflection a
separate bone attachment. Hidden, muted clients compare the source, a full skin
rewrite, and a deliberately incorrect surface-following control at two poses.
All files and profiles belong to the requested new output directory.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import re
import struct
import subprocess
import time

from PIL import Image
from audit_native_item_scripts import DatView
from review_native_css_room import Client


def sphere(path):
    positions, normals, uv, indices = [], [], [], []
    rows, columns = 16, 32
    for row in range(rows+1):
        phi = row*math.pi/rows
        for column in range(columns+1):
            theta = column*2*math.pi/columns
            normal = math.sin(phi)*math.cos(theta), math.cos(phi), math.sin(phi)*math.sin(theta)
            normals.extend(normal)
            positions.extend(center+4*value for center, value in zip((-10, 13, 4), normal))
            uv.extend((column/columns, row/rows))
    for row in range(rows):
        for column in range(columns):
            a = row*(columns+1)+column
            b = a+columns+1
            indices.extend((a, a+1, b, a+1, b+1, b))
    arrays = [struct.pack('<%df'%len(v), *v) for v in (positions, normals, uv)]
    arrays.append(struct.pack('<%dH'%len(indices), *indices))
    binary = b''.join(arrays)
    views, offset = [], 0
    for array in arrays:
        views.append(dict(buffer=0, byteOffset=offset, byteLength=len(array)))
        offset += len(array)
    count = len(positions)//3
    document = dict(asset=dict(version='2.0'), buffers=[dict(byteLength=len(binary))],
        bufferViews=views, accessors=[
            dict(bufferView=0, componentType=5126, count=count, type='VEC3', min=[-14,9,0], max=[-6,17,8]),
            dict(bufferView=1, componentType=5126, count=count, type='VEC3'),
            dict(bufferView=2, componentType=5126, count=count, type='VEC2'),
            dict(bufferView=3, componentType=5123, count=len(indices), type='SCALAR')],
        meshes=[dict(name='Independent reflection', primitives=[dict(
            attributes=dict(POSITION=0, NORMAL=1, TEXCOORD_0=2), indices=3, mode=4)])],
        nodes=[dict(mesh=0)], scenes=[dict(nodes=[0])], scene=0)
    encoded = json.dumps(document).encode()
    encoded += b' '*(-len(encoded)%4)
    binary += b'\0'*(-len(binary)%4)
    path.write_bytes(struct.pack('<III',0x46546c67,2,28+len(encoded)+len(binary))+
                    struct.pack('<II',len(encoded),0x4e4f534a)+encoded+
                    struct.pack('<II',len(binary),0x004e4942)+binary)


def texture_fixture(full, joints, follow=False, distinct_basis=False):
    """Author two canonical envelopes and add a per-corner texture selector."""
    data_size, relocation_count = struct.unpack_from('>II', full, 4)
    data = bytearray(full[32:32+data_size])
    links = {struct.unpack_from('>I', full, 32+data_size+i*4)[0] for i in range(relocation_count)}
    tail = full[32+data_size+relocation_count*4:]
    def word(at): return struct.unpack_from('>I', data, at)[0]
    def put(at, value): struct.pack_into('>I', data, at, value)
    def allocate(size, alignment=4):
        at = (len(data)+alignment-1)&-alignment
        data.extend(b'\0'*(at+size-len(data)))
        return at
    def link(at, target):
        put(at, target)
        links.add(at)
    mesh = word(joints[0]+16)
    assert mesh and word(mesh+8), 'Imported root mesh/material missing'
    material = word(mesh+8)
    texture = word(material+8)
    assert texture, 'Fixture needs its generated texture'
    # Reflection coordinates, replace RGB, preserve other alpha/lightmap flags.
    put(texture+64, (word(texture+64)&~0xf000f)|1|(5<<16))
    if joints[0]+56 not in links:
        bind = allocate(48)
        for axis in range(3): struct.pack_into('>f', data, bind+(axis*4+axis)*4, 1)
        link(joints[0]+56, bind)
    assert joints[4]+56 in links, 'Choose a deform bone with an inverse bind'
    if distinct_basis:
        # A real legacy rigid shortcut reads normals before the root bind
        # conversion. Shear and nonuniform scale make lost basis data visible.
        bind = word(joints[0]+56)
        for row, values in enumerate(((1.2,.25,0),(0,.8,.1),(0,0,1.1))):
            for col,value in enumerate(values): struct.pack_into('>f',data,bind+(row*4+col)*4,value)
    for polygon in polygon_chain(data, word(mesh+12)):
        old = word(polygon+8)
        assert [word(old+i*24) for i in range(5)] == [0,9,10,13,255]
        assert all(word(old+i*24+4)==3 for i in (1,2,3))
        attrs = allocate(6*24)
        for source_index, target_index in ((0,0),(1,2),(2,3),(3,4)):
            source, target = old+source_index*24, attrs+target_index*24
            data[target:target+24] = data[source:source+24]
            if source+20 in links: link(target+20, word(source+20))
        put(attrs+24,1);put(attrs+28,1);put(attrs+5*24,255)
        link(polygon+8, attrs)
        table = allocate(12)
        for slot, bone in enumerate((joints[0], joints[4])):
            entries = allocate(24)
            link(table+slot*4, entries)
            for i in range(2):
                link(entries+i*8, bone)
                weight = (1.0 if i==0 else 0.0) if distinct_basis and slot==0 else .5
                struct.pack_into('>f', data, entries+i*8+4, weight)
        link(polygon+20, table)
        struct.pack_into('>H', data, polygon+12,
                         (struct.unpack_from('>H',data,polygon+12)[0]&~0x3000)|0x2000)
        start = word(polygon+16)
        end = start+struct.unpack_from('>H',data,polygon+14)[0]*32
        commands = bytearray()
        cursor = start
        while cursor < end:
            command = data[cursor];cursor += 1
            if not command: continue
            assert command == 0x90
            count = struct.unpack_from('>H',data,cursor)[0];cursor += 2
            commands.extend(struct.pack('>BH',command,count))
            for _ in range(count):
                commands.extend((0,30 if follow else 33))
                commands.extend(data[cursor+1:cursor+7])
                cursor += 7
        commands.extend(b'\0'*(-len(commands)%32))
        display = allocate(len(commands),32)
        data[display:display+len(commands)] = commands
        link(polygon+16,display)
        struct.pack_into('>H',data,polygon+14,len(commands)//32)
    header = bytearray(full[:32])
    struct.pack_into('>III',header,0,32+len(data)+len(links)*4+len(tail),len(data),len(links))
    return bytes(header)+data+b''.join(struct.pack('>I',at) for at in sorted(links))+tail


def polygon_chain(data, first):
    while first:
        yield first
        first = struct.unpack_from('>I',data,first+4)[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe','tool','test','iso','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--distinct-basis',action='store_true')
    args = parser.parse_args()
    output = args.output.resolve();output.mkdir(parents=True,exist_ok=False)
    profile = output/'profile'
    draft = profile/'Freak Melee/projects/texture-fixture'
    (profile/'Freak Melee/content').mkdir(parents=True)
    draft.parent.mkdir(parents=True)
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = 'http://127.0.0.1:1'
    env = dict(os.environ, LOCALAPPDATA=str(output/'tool-profile'))
    receipt = dict(status='RUNNING', binaries={p.name:hashlib.sha256(p.read_bytes()).hexdigest()
        for p in (args.exe,args.tool,args.test)}, captures={}, comparisons={})
    def tool(*parts):
        process = subprocess.run([str(args.tool.resolve()),*map(str,parts)],env=env,
            capture_output=True,text=True,creationflags=subprocess.CREATE_NO_WINDOW)
        with (output/'tool.log').open('a') as log:
            log.write(' '.join(map(str,parts))+'\n'+process.stdout+process.stderr)
        assert process.returncode==0,process.stdout+process.stderr
        return process.stdout
    tool('clone-stock',args.iso.resolve(),'Mario',draft,'Texture attachment review')
    source = output/'sphere.glb';sphere(source)
    tool('project-model-import',draft,source,0,0,'add',1,'auto')
    image = Image.new('RGBA',(128,128))
    for y in range(128):
        for x in range(128):
            image.putpixel((x,y),(30+int(x*220/127),30+int(y*220/127),
                                 240 if (x//16+y//16)%2 else 20,255))
    image.save(output/'reflection.png')
    tool('project-model-texture',draft,0,output/'reflection.png')
    joints = [int(row,16) for row in re.findall(r'joint \d+ parent -?\d+ offset 0x([\dA-Fa-f]+)',
                                               tool('project-model-joints',draft))]
    assert len(joints)>4
    plain = (draft/'costume.dat').read_bytes()
    original = texture_fixture(plain,joints,distinct_basis=args.distinct_basis)
    broken = texture_fixture(plain,joints,follow=True,distinct_basis=args.distinct_basis)
    (output/'original.dat').write_bytes(original)
    (output/'negative-control.dat').write_bytes(broken)
    # Use the actual skin writer and compactor, not a Python approximation.
    roots = [name for name in DatView(original).roots if name.endswith('_Share_joint')]
    assert len(roots)==1
    root = roots[0]
    process = subprocess.run([str(args.test.resolve()),str(output/'original.dat'),root,
                              str(output/'rewritten.dat')],capture_output=True,text=True,
                             creationflags=subprocess.CREATE_NO_WINDOW)
    (output/'rewrite.log').write_text(process.stdout+process.stderr)
    assert process.returncode==0,process.stdout+process.stderr
    variants = dict(original=original, rewritten=(output/'rewritten.dat').read_bytes(), negative=broken)
    def surface_box(client):
        start=client.stderr.stat().st_size
        client.command('ui-inspect','workshop-model-surface')
        deadline=time.monotonic()+10
        while time.monotonic()<deadline:
            with client.stderr.open(errors='replace') as file:file.seek(start);text=file.read()
            count=re.search(r'ready=1 vertices=(\d+)',text)
            points=re.findall(r'visible=1 screen=([\d.e+-]+),([\d.e+-]+)',text)
            if count and text.count('[workshop-model-vertex]')==int(count[1]):break
            time.sleep(.1)
        assert count and len(points)>20,text[-1500:]
        left,top,width,height=map(float,re.search(r'origin=([\d.-]+),([\d.-]+) size=([\d.-]+),([\d.-]+)',
                                                 client.inspect('workshop-model-viewport')).groups())
        h=min(height,width*.75);w=h*4/3;left+=(width-w)/2;top+=(height-h)/2
        pixels=[(left+float(x)*w,top+float(y)*h) for x,y in points]
        box=(max(math.floor(left),math.floor(min(p[0] for p in pixels))-2),
             max(math.floor(top),math.floor(min(p[1] for p in pixels))-2),
             min(math.ceil(left+w),math.ceil(max(p[0] for p in pixels))+2),
             min(math.ceil(top+h),math.ceil(max(p[1] for p in pixels))+2))
        assert box[2]-box[0]>20 and box[3]-box[1]>20,box
        return box
    for label, costume in variants.items():
        (draft/'costume.dat').write_bytes(costume)
        client = Client(args.exe.resolve(),args.iso.resolve(),profile,output/label,
                        'Texture Review',window_size=(1440,900))
        try:
            client.click('nav-5');client.click('import-fighter');client.click('workshop-draft-0')
            client.element('workshop-editor-header')
            deadline=time.monotonic()+60
            while time.monotonic()<deadline:
                if 'text=Draft preview ready.' in client.inspect('workshop-editor-status'):break
                time.sleep(.2)
            else:raise RuntimeError('Frozen preview did not load')
            client.click('workshop-mode-model')
            client.click('workshop-model-context-skeleton');client.click('workshop-model-rig-pose')
            client.click('workshop-model-joint-4')
            initial = float(re.search(r'value=([^ ]+)',client.inspect('workshop-model-rot-2'))[1])
            for pose, angle in (('rest',initial),('rotated',initial+40)):
                client.enter('workshop-model-rot-2',str(angle));client.click('workshop-model-apply-transform')
                client.click('workshop-model-context-meshes')
                time.sleep(.6)
                receipt.setdefault('boxes',{})[label+'-'+pose]=surface_box(client)
                receipt['captures'][label+'-'+pose] = str(client.screenshot())
                if pose=='rest':client.click('workshop-model-context-skeleton')
            receipt.setdefault('clients_stopped',[])
            logs=(client.folder/'stdout.log').read_text(errors='replace')+client.stderr.read_text(errors='replace')
            assert not re.search(r'Fighter source VM failed|native panic stack|Shader compilation failed|device lost',
                                 logs,re.IGNORECASE),logs[-2000:]
        finally:
            client.stop();receipt.setdefault('clients_stopped',[]).append(client.process.poll() is not None)
            (output/'receipt.json').write_text(json.dumps(receipt,indent=2))
    # Derive the fixture's bounds from the runtime projection of its vertices.
    for pose in ('rest','rotated'):
        box=receipt['boxes']['original-'+pose]
        a=Image.open(receipt['captures']['original-'+pose]).convert('RGB').crop(box)
        for other in ('rewritten','negative'):
            assert all(abs(x-y)<=1 for x,y in zip(box,receipt['boxes'][other+'-'+pose])),receipt['boxes']
            b=Image.open(receipt['captures'][other+'-'+pose]).convert('RGB').crop(box)
            differences=[max(abs(x-y) for x,y in zip(p,q)) for p,q in zip(a.getdata(),b.getdata())]
            receipt['comparisons'][other+'-'+pose]=dict(changed=sum(d>3 for d in differences),
                                                       maximum=max(differences),mean=sum(differences)/len(differences))
    assert all(receipt['comparisons']['rewritten-'+p]['maximum']==0 for p in ('rest','rotated')),receipt['comparisons']
    assert receipt['comparisons']['negative-rotated']['changed']>1000,receipt['comparisons']
    receipt['status']='PASS'
    (output/'receipt.json').write_text(json.dumps(receipt,indent=2))
    print(json.dumps(receipt,indent=2))


if __name__=='__main__': main()
