"""Check compatible glTF skin import against real fighter bind data.

Builds isolated GLB inputs, invokes the native import tool, and reads saved DAT
envelopes independently. Negative cases compare every project file hash.
"""
import argparse
import copy
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

from review_native_skin_weights import POSITIONS, raw_mesh


IDENTITY = [[1.,0,0,0],[0,1.,0,0],[0,0,1.,0]]


def mul(a,b):
    return [[sum(a[r][k]*b[k][c] for k in range(3))+(a[r][3] if c==3 else 0)
             for c in range(4)] for r in range(3)]


def inverse(m):
    a,b,c,d,e,f,g,h,i = [m[r][c] for r in range(3) for c in range(3)]
    det=a*(e*i-f*h)-b*(d*i-f*g)+c*(d*h-e*g)
    assert abs(det)>1e-12
    linear=[[e*i-f*h,c*h-b*i,b*f-c*e],
            [f*g-d*i,a*i-c*g,c*d-a*f],
            [d*h-e*g,b*g-a*h,a*e-b*d]]
    result=[[x/det for x in row]+[0] for row in linear]
    for r in range(3): result[r][3]=-sum(result[r][k]*m[k][3] for k in range(3))
    return result


def column_major(m):
    return [m[r][c] if r<3 else float(c==3) for c in range(4) for r in range(4)]


def hashes(project):
    return {p.relative_to(project).as_posix():hashlib.sha256(p.read_bytes()).hexdigest()
            for p in project.rglob('*') if p.is_file()}


class Glb:
    def __init__(self):
        self.binary=bytearray()
        self.doc=dict(asset=dict(version='2.0'),buffers=[],bufferViews=[],accessors=[])

    def view(self,data,stride=None):
        self.binary.extend(b'\0'*(-len(self.binary)%4))
        v=dict(buffer=0,byteOffset=len(self.binary),byteLength=len(data))
        if stride:v['byteStride']=stride
        self.doc['bufferViews'].append(v)
        self.binary.extend(data)
        return len(self.doc['bufferViews'])-1

    def accessor(self,values,component,count,kind,normalized=False,sparse=False):
        fmt={5121:'B',5123:'H',5126:'f'}[component]
        data=struct.pack('<'+fmt*len(values),*values)
        a=dict(componentType=component,count=count,type=kind)
        if normalized:a['normalized']=True
        if sparse:
            a['sparse']=dict(count=count,indices=dict(bufferView=self.view(bytes(range(count))),
                componentType=5121),values=dict(bufferView=self.view(data)))
        else:a['bufferView']=self.view(data)
        self.doc['accessors'].append(a)
        return len(self.doc['accessors'])-1

    def save(self,path):
        self.binary.extend(b'\0'*(-len(self.binary)%4))
        self.doc['buffers']=[dict(byteLength=len(self.binary))]
        text=json.dumps(self.doc).encode()
        text+=b' '*(-len(text)%4)
        path.write_bytes(struct.pack('<III',0x46546c67,2,28+len(text)+len(self.binary))+
            struct.pack('<II',len(text),0x4e4f534a)+text+
            struct.pack('<II',len(self.binary),0x004e4942)+self.binary)


def fixture(path,joints,shape,unit=1,weight_type='float',influences=8,
            fault=None,sparse=False,interleaved=False,first_bones=False):
    g=Glb()
    pos=g.accessor([x/unit for p in POSITIONS for x in p],5126,4,'VEC3')
    g.doc['accessors'][pos].update(min=[min(p[c] for p in POSITIONS)/unit for c in range(3)],
                                  max=[max(p[c] for p in POSITIONS)/unit for c in range(3)])
    normal=g.accessor([0,0,1]*4,5126,4,'VEC3')
    indices=g.accessor([0,1,2,0,2,3],5123,6,'SCALAR')
    # Skin-table order deliberately differs from node order and fighter order.
    order=list(reversed(range(len(joints))))
    source_inverse=[]
    for joint in joints:
        matrix=mul(joint['inverse'],shape)
        for r in range(3):matrix[r][3]/=unit
        source_inverse.append(matrix)
    binds=[inverse(m) for m in source_inverse]
    targets={j['id']:i for i,j in enumerate(joints)}
    nodes=[dict(name='Armature',children=[])]
    for at,joint in enumerate(joints):
        parent=joint['parent']
        while parent>=0 and parent not in targets:parent=joint['parents'][parent]
        local=mul(source_inverse[targets[parent]],binds[at]) if parent>=0 else binds[at]
        nodes.append(dict(name=joint['name'],matrix=column_major(local),children=[]))
    for at,joint in enumerate(joints):
        parent=joint['parent']
        while parent>=0 and parent not in targets:parent=joint['parents'][parent]
        nodes[targets[parent]+1 if parent>=0 else 0]['children'].append(at+1)
    mesh_node=len(nodes)
    # This transform is ignored by glTF skinning and must not be baked again.
    nodes.append(dict(name='Skinned mesh',mesh=0,skin=0,translation=[500,40,-80]))
    floats=[.1234567,.2,.13,.1,.15,.08,.07,.1465433] if influences==8 else [1/influences]*influences
    if weight_type=='u8':base=[1,2,3,4,5,6,7,227];component=5121;denom=255
    elif weight_type=='u16':base=[1024,2048,4096,8192,16384,100,200,33491];component=5123;denom=65535
    else:base=floats;component=5126;denom=1
    source_indices=list(range(influences))
    if first_bones:source_indices=[len(joints)-1-i for i in source_indices]
    if fault=='duplicate-influence':source_indices[-1]=source_indices[0]
    expected={}
    for i,w in zip(source_indices,base):
        bone=joints[order[i]]['id']
        expected[bone]=expected.get(bone,0)+w/denom
    total=sum(expected.values());expected={j:w/total for j,w in expected.items()}
    count=(influences+3)//4
    data=[]
    for s in range(count):
        ji=(source_indices[s*4:s*4+4]+[0]*4)[:4]
        we=(base[s*4:s*4+4]+[0]*4)[:4]
        if fault=='invalid-index' and s==0:ji[0]=65535
        if fault=='negative-weight' and s==0:we[0]=-.2
        if fault=='nan-weight' and s==0:we[0]=float('nan')
        if fault=='zero-weight':we=[0]*4
        data.append((ji,we))
    attributes=dict(POSITION=pos,NORMAL=normal)
    if interleaved:
        blob=bytearray()
        for v in range(4):
            for ji,we in data:blob.extend(bytes(ji)+bytes(we))
        view=g.view(blob,8*count)
        for s in range(count):
            for suffix,offset,norm in [('JOINTS',s*8,False),('WEIGHTS',s*8+4,True)]:
                a=dict(bufferView=view,byteOffset=offset,componentType=5121,count=4,type='VEC4')
                if norm:a['normalized']=True
                g.doc['accessors'].append(a)
                attributes[f'{suffix}_{s}']=len(g.doc['accessors'])-1
    else:
        for s,(ji,we) in enumerate(data):
            attributes[f'JOINTS_{s}']=g.accessor(ji*4,5123,4,'VEC4',sparse=sparse)
            attributes[f'WEIGHTS_{s}']=g.accessor(we*4,component,4,'VEC4',
                normalized=component!=5126,sparse=sparse)
    if fault=='missing-pair':del attributes['WEIGHTS_1']
    ibms=[copy.deepcopy(source_inverse[i]) for i in order]
    if fault=='bind-mismatch':ibms[1][0][3]+=3/unit
    if fault=='singular-bind':ibms[0][0]=[0,0,0,0]
    matrices=[column_major(m) for m in ibms]
    if fault=='perspective-bind':matrices[0][3]=.25
    inverse_accessor=g.accessor([x for m in matrices for x in m],5126,len(joints),'MAT4')
    skin=dict(joints=[i+1 for i in order],inverseBindMatrices=inverse_accessor,skeleton=0)
    if fault=='no-inverse':del skin['inverseBindMatrices']
    if fault=='unmapped':nodes[order[0]+1]['name']='Not a fighter bone'
    if fault=='unnamed':del nodes[order[0]+1]['name']
    if fault=='duplicate-bone':nodes[order[1]+1]['name']=nodes[order[0]+1]['name']
    if fault=='hierarchy':
        child=next(i for i in range(1,mesh_node) if any(i in n.get('children',[]) for n in nodes[1:mesh_node]))
        for n in nodes:n['children']=[c for c in n.get('children',[]) if c!=child]
        nodes[0]['children'].append(child)
    if fault=='no-skin':del nodes[mesh_node]['skin']
    # A posed source skeleton still uses its bind data for this rest-rig import.
    if fault=='posed':nodes[order[0]+1]['matrix']=column_major(IDENTITY)
    primitive=dict(attributes=attributes,indices=indices,mode=4)
    g.doc.update(nodes=nodes,meshes=[dict(name='Source skin fixture',primitives=[primitive])],
        skins=[skin],scenes=[dict(nodes=[0,mesh_node])],scene=0)
    if fault=='batch-failure':
        bad=copy.deepcopy(primitive)
        bad['attributes']['WEIGHTS_0']=g.accessor([-.2,.4,.4,.4]*4,5126,4,'VEC4')
        g.doc['meshes'].append(dict(name='Invalid second mesh',primitives=[bad]))
        g.doc['nodes'].append(dict(mesh=1,skin=0))
        g.doc['scenes'][0]['nodes'].append(len(nodes)-1)
    g.save(path)
    placed=[tuple(sum(shape[r][k]*p[k] for k in range(3))+shape[r][3] for r in range(3))
            for p in POSITIONS]
    return placed,expected


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ['tool','draft','output']:parser.add_argument('--'+name,type=Path,required=True)
    for name in ['exe','iso','control']:parser.add_argument('--'+name,type=Path)
    args=parser.parse_args();output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
    if args.exe and not args.iso:parser.error('--exe requires --iso')
    project=output/'draft';shutil.copytree(args.draft.resolve(),project)
    tool=args.tool.resolve();env=dict(os.environ,LOCALAPPDATA=str(output/'appdata'))
    def command(*parts,success=True):
        r=subprocess.run([str(tool),*map(str,parts)],cwd=output,env=env,capture_output=True,
                         text=True,encoding='utf-8',creationflags=subprocess.CREATE_NO_WINDOW)
        with (output/'tool.log').open('a') as log:log.write(' '.join(map(str,parts))+'\n'+r.stdout+r.stderr)
        assert (r.returncode==0)==success,r.stdout+r.stderr
        return r.stdout+r.stderr
    matches=re.findall(r'joint (\d+) parent (-?\d+) offset 0x([0-9A-Fa-f]+) flags 0x([0-9A-Fa-f]+) (.+)',
                       command('project-model-joints',project))
    parents=[int(m[1]) for m in matches];offsets=[int(m[2],16) for m in matches]
    raw=(project/'costume.dat').read_bytes()[32:];joints=[]
    for m in matches:
        at,parent,offset,flags=int(m[0]),int(m[1]),int(m[2],16),int(m[3],16)
        ptr=struct.unpack_from('>I',raw,offset+56)[0]
        if not flags&1 or not ptr:continue
        values=struct.unpack_from('>12f',raw,ptr)
        joints.append(dict(id=at,parent=parent,parents=parents,offset=offset,
            name=m[4].split(' · joint ')[0],inverse=[list(values[i*4:i*4+4]) for i in range(3)]))
    assert len(joints)>17
    angle=.37;c,s=math.cos(angle),math.sin(angle)
    shape=[[c,-s,0,4],[s,c,0,-3],[0,0,1,2]]
    results=[]
    positives=[('float',{}),('u8',dict(weight_type='u8')),('u16',dict(weight_type='u16')),
               ('interleaved',dict(weight_type='u8',interleaved=True)),('sparse',dict(sparse=True)),
               ('duplicate',dict(fault='duplicate-influence')),('sixteen',dict(influences=16)),
               ('units',dict(unit=100)),('posed',dict(fault='posed'))]
    for case_index,(label,options) in enumerate(positives):
        case_shape=copy.deepcopy(shape)
        case_shape[0][3]+=case_index*3
        source=output/(label+'.glb');placed,expected=fixture(source,joints,case_shape,**options)
        command('project-model-import',project,source,0,0,'add',options.get('unit',1),'source')
        mesh,weights,_,_,_=raw_mesh(project/'costume.dat',offsets,expected_positions=placed,tolerance=.0001)
        for actual in weights.values():
            assert actual.keys()==expected.keys(),(label,actual,expected)
            assert all(abs(actual[j]-expected[j])<3e-7 for j in expected),(label,actual,expected)
        command('project-model-vertices',project,mesh)
        retained=copy.deepcopy(weights)
        command('project-model-import',project,source,0,0,mesh,options.get('unit',1),'keep')
        _,preserved,_,_,_=raw_mesh(project/'costume.dat',offsets,mesh,expected_positions=placed,tolerance=.0001)
        assert preserved==retained,(label,'reimport changed source weights',retained,preserved)
        results.append(dict(case=label,status='PASS',mesh=mesh,influences=len(expected)))
    negatives=['unmapped','unnamed','duplicate-bone','hierarchy','bind-mismatch',
               'singular-bind','perspective-bind','invalid-index','negative-weight',
               'nan-weight','zero-weight','missing-pair','no-skin','batch-failure','capacity','wrong-units']
    for fault in negatives:
        source=output/(fault+'.glb');options={}
        if fault=='capacity':options['influences']=17
        elif fault=='wrong-units':options['unit']=100
        else:options['fault']=fault
        fixture(source,joints,shape,**options)
        before=hashes(project)
        if fault=='batch-failure':message=command('project-import-meshes',project,source,0,1,'source',success=False)
        else:message=command('project-model-import',project,source,0,0,'add',1,'source',success=False)
        assert hashes(project)==before,fault+' changed project data'
        results.append(dict(case=fault,status='PASS rejection',diagnostic=message.strip()))
    command('source-check',project,project/'behavior.fms')
    if args.exe:
        from review_native_css_room import Client
        profile=output/'preview-profile'
        (profile/'Freak Melee/content').mkdir(parents=True)
        destination=profile/'Freak Melee/projects/source-skin-review'
        destination.parent.mkdir()
        shutil.copytree(args.draft.resolve(),destination)
        source=output/'wizard.glb'
        placed,expected=fixture(source,joints,IDENTITY,first_bones=True)
        client=Client(args.exe.resolve(),args.iso.resolve(),profile,output/'preview',
            'Source Skin Review',window_size=(1440,810),
            control=args.control.resolve() if args.control else None)
        try:
            client.click('edit-fighter')
            client.element('workshop-editor-header')
            def preview_ready():
                deadline=time.monotonic()+60
                while time.monotonic()<deadline:
                    if 'text=Draft preview ready.' in client.inspect('workshop-editor-status'):return
                raise RuntimeError('Draft preview did not become ready')
            preview_ready()
            client.click('workshop-mode-model')
            client.click('workshop-model-import')
            client.enter('workshop-model-import-path',str(source))
            client.click('workshop-model-import-load')
            client.click('workshop-model-import-next')
            client.click('workshop-model-import-next')
            binding=client.inspect('workshop-model-import-binding')
            assert 'value=source' in binding,binding
            client.screenshot()
            client.click('workshop-model-import-next')
            client.click('workshop-save-top')
            client.click('workshop-mode-action')
            deadline=time.monotonic()+60
            while time.monotonic()<deadline:
                if 'paused preview' in client.inspect('workshop-preview-state'):break
            else:raise RuntimeError('Imported fighter did not return to its frozen pose')
            client.click('workshop-mode-model')
            saved=client.data/'projects/source-skin-review'
            mesh,weights,_,_,_=raw_mesh(saved/'costume.dat',offsets,expected_positions=placed,tolerance=.0001)
            for actual in weights.values():
                assert actual.keys()==expected.keys(),(actual,expected)
                assert all(abs(actual[j]-expected[j])<3e-7 for j in expected)
            client.click('workshop-model-context-skinning')
            client.click(f'workshop-model-joint-{max(expected,key=expected.get)}')
            client.screenshot()
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
            results.append(dict(case='native-wizard-preview-and-test-character',status='PASS',mesh=mesh))
        finally:client.stop()
    (output/'passed.json').write_text(json.dumps(dict(status='PASS',cases=results,
        tool_sha256=hashlib.sha256(tool.read_bytes()).hexdigest(),project_files=hashes(project)),indent=2)+'\n')
    print(f'PASS: {len(positives)} source-skin imports/weight-preserving reimports and {len(negatives)} atomic rejection cases')


if __name__=='__main__':main()
