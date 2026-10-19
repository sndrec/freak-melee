"""Review matched reimport, painted weights and material choices in the native Workshop.

Uses a copied draft, a hidden muted client and local control input. Saved DAT
contents provide the geometry/weight/material checks; screenshots record the UI.
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

from review_native_css_room import Client
from review_native_gltf_sources import fixture
from review_native_skin_weights import POSITIONS, raw_mesh

ASSETS=('fighter.dat','animation.dat','costume.dat','assets.fma','workshop_names.txt')


def snapshot(path):
    return {name:(path/name).read_bytes() for name in ASSETS if (path/name).exists()}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','tool','control','iso','draft','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args();output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
    profile=output/'profile';draft=profile/'Freak Melee/projects/reimport-review'
    shutil.copytree(args.draft.resolve(),draft)
    (profile/'Freak Melee/content').mkdir()
    original=snapshot(args.draft)
    paths={}
    for name,options in [('original',{}),('deformed',dict(deformed=True)),
                         ('material',dict(deformed=True,rename_material=True)),
                         ('topology',dict(topology=True)),('missing',dict(missing=True))]:
        paths[name]=output/(name+'.glb');fixture(paths[name],**options)
    receipt=dict(status='RUNNING',cases=[],screenshots=[])
    client=None

    def tool(*parts):
        r=subprocess.run([str(args.tool.resolve()),*map(str,parts)],capture_output=True,text=True,
            encoding='utf-8',errors='replace',creationflags=subprocess.CREATE_NO_WINDOW,
            env=dict(os.environ,LOCALAPPDATA=str(output/'tool-profile')))
        assert r.returncode==0,r.stdout+r.stderr
        return r.stdout

    def sources(path):
        return {identity:(int(ordinal),int(offset,16)) for ordinal,offset,identity in
            re.findall(r'^(\d+) mesh=0x([0-9A-Fa-f]+).* identity=(.*?) path=',
                       tool('project-model-sources',path),re.M)}

    def materials(path,offset):
        data=(path/'costume.dat').read_bytes()[32:]
        word=lambda at:struct.unpack_from('>I',data,at)[0]
        mobj=word(offset+8);color=word(mobj+12)
        return mobj,tuple(data[color+4:color+8]),struct.unpack_from('>f',data,color+16)[0]

    def weights(path,key,positions):
        mesh,_=sources(path)[key]
        return raw_mesh(path/'costume.dat',joint_offsets,mesh,expected_positions=positions)[1]

    def ready(c):
        start=(c.folder/'stdout.log').stat().st_size;deadline=time.monotonic()+60
        while time.monotonic()<deadline:
            c.command('ui-inspect','workshop-preview');time.sleep(.15)
            with (c.folder/'stdout.log').open(errors='replace') as log:
                log.seek(start);text=log.read()
            if 'editing=1 live=0 transitioning=0' in text and '[workshop-camera] active=1' in text:return
        raise RuntimeError('Preview did not become ready')

    def launch(folder):
        c=Client(args.exe.resolve(),args.iso.resolve(),profile,output/folder,
            'Reimport Review',window_size=(1440,900),control=args.control.resolve())
        try:
            c.click('nav-5');c.click('edit-fighter');c.element('workshop-editor-header');ready(c)
            c.click('workshop-mode-model');c.click('workshop-model-context-meshes')
            return c
        except BaseException:
            c.stop();raise

    def select(c,id,value):
        c.click(id)
        state=c.inspect(id)
        options=re.findall(r'\[ui-inspect-option\] '+re.escape(id)+r' index=(\d+) value=(\S+) text=',state)
        values=[v for _,v in options]
        current=re.search(r'\[ui-inspect-value\] '+re.escape(id)+r':.* value=(\S+) text=',state)[1]
        delta=values.index(value)-values.index(current)
        for _ in range(abs(delta)):c.command('ui-key','down' if delta>0 else 'up')
        c.command('ui-key','enter')
        assert re.search(r'\[ui-inspect-value\] '+re.escape(id)+r':.* value='+re.escape(value)+r' text=',c.inspect(id))

    def save(c):
        c.click('workshop-save-top');assert 'Draft saved.' in c.inspect('workshop-save-status')
        return snapshot(c.data/'projects/reimport-review')

    def open_reimport(c,path):
        mesh,_=sources(c.data/'projects/reimport-review')['mesh:4:Hull']
        c.click('workshop-model-mesh-'+str(mesh));c.click('workshop-model-reimport')
        c.enter('workshop-model-import-path',str(path));c.click('workshop-model-import-load')

    def apply(c,success=True):
        c.click('workshop-model-import-next')
        visible='visible=1' in c.inspect('workshop-model-import-overlay')
        if success:assert not visible,c.inspect('workshop-model-import-note');ready(c)
        else:assert visible and 'topology changed' in c.inspect('workshop-model-import-note')

    try:
        tool('project-model-import',draft,paths['original'],0,0,'add',1,'auto')
        tool('project-model-import',draft,paths['original'],1,0,'add',2,'auto')
        found=sources(draft);hull,hull_offset=found['mesh:4:Hull'];_,trim_offset=found['mesh:4:Trim']
        joints=re.findall(r'joint (\d+) parent -?\d+ offset 0x([0-9A-Fa-f]+) flags 0x([0-9A-Fa-f]+)',
                          tool('project-model-joints',draft))
        joint_offsets=[int(at,16) for _,at,_ in joints]
        data=(draft/'costume.dat').read_bytes()[32:]
        bone=next(int(i) for i,at,flags in joints if int(flags,16)&1 and struct.unpack_from('>I',data,int(at,16)+56)[0])
        tool('project-model-paint',draft,hull,bone,*POSITIONS[0],.5,.375,'add')
        # Fixture: two meshes share a material that has been edited in the project.
        data=bytearray((draft/'costume.dat').read_bytes());word=lambda at:struct.unpack_from('>I',data,32+at)[0]
        mobj=word(hull_offset+8);color=word(mobj+12)
        struct.pack_into('>I',data,32+trim_offset+8,mobj)
        data[32+color+4]=181;struct.pack_into('>f',data,32+color+16,19.25)
        (draft/'costume.dat').write_bytes(data)
        initial_hull=weights(draft,'mesh:4:Hull',POSITIONS)
        trim_positions=[((x+3)*2,y*2,z*2) for x,y,z in POSITIONS]
        initial_trim=weights(draft,'mesh:4:Trim',trim_positions)
        client=launch('editing');live=client.data/'projects/reimport-review';baseline=save(client)
        open_reimport(client,paths['missing']);client.click('workshop-model-reimport-whole')
        assert 'missing or ambiguous' in client.inspect('workshop-model-import-note')
        assert snapshot(live)==baseline
        client.enter('workshop-model-import-path',str(paths['deformed']));client.click('workshop-model-import-load')
        client.click('workshop-model-import-next')
        for i in range(2):
            assert 'value=' in client.inspect('workshop-model-reimport-scale-'+str(i))
        receipt['screenshots'].append(str(client.screenshot()))
        client.click('workshop-model-import-next')
        assert re.search(r'\[ui-inspect-value\] workshop-model-import-binding:.* value=keep text=',
                         client.inspect('workshop-model-import-binding'))
        receipt['screenshots'].append(str(client.screenshot()))
        apply(client);after=save(client)
        placed=[(x+1.25,y+2,z) for x,y,z in POSITIONS]
        trim_placed=[((x+4.25)*2,(y+2)*2,z*2) for x,y,z in POSITIONS]
        assert weights(live,'mesh:4:Hull',placed)==initial_hull
        assert weights(live,'mesh:4:Trim',trim_placed)==initial_trim
        assert materials(live,hull_offset)==materials(live,trim_offset)==(mobj,(181,255,255,255),19.25)
        receipt['cases'].append('Whole-source matching retains painted weights, separate scales and edited shared material')
        receipt['screenshots'].append(str(client.screenshot()))
        client.click('workshop-undo');assert save(client)==baseline
        client.click('workshop-redo');assert save(client)==after
        receipt['cases'].append('Whole reimport has byte-exact geometry and metadata Undo/Redo')
        open_reimport(client,paths['material']);client.click('workshop-model-import-next')
        select(client,'workshop-model-import-materials','replace');client.click('workshop-model-import-next')
        apply(client);save(client)
        new_material=materials(live,hull_offset)
        assert new_material[0]!=mobj and new_material[1]==(51,102,153,255)
        assert materials(live,trim_offset)==(mobj,(181,255,255,255),19.25)
        assert weights(live,'mesh:4:Hull',placed)==initial_hull
        receipt['cases'].append('Selected source-material replacement retains weights and the other shared material user')
        before_topology=snapshot(live)
        open_reimport(client,paths['topology']);client.click('workshop-model-import-next');client.click('workshop-model-import-next')
        apply(client,False);assert snapshot(live)==before_topology
        select(client,'workshop-model-import-binding','transfer');apply(client);final=save(client)
        receipt['cases'].append('Changed topology requires an explicit weight-transfer choice')
        shutil.copytree(live,draft,dirs_exist_ok=True);client.stop();client=None
        for path in paths.values():path.rename(path.with_suffix('.held'))
        client=launch('reopened');assert save(client)==final
        receipt['screenshots'].append(str(client.screenshot()))
        client.click('workshop-test-character');client.element('workshop-return-editor');time.sleep(3)
        receipt['screenshots'].append(str(client.screenshot()))
        client.click('workshop-return-editor');client.element('workshop-editor-header')
        receipt['cases'].append('Saved reimport reopens without source files and runs in Test Character')
        receipt['status']='PASS'
    except BaseException as error:
        receipt['status']='FAIL';receipt['error']=repr(error)
        if client:receipt['screenshots'].append(str(client.screenshot()))
        raise
    finally:
        if client:client.stop()
        assert snapshot(args.draft)==original
        receipt['source_unchanged']=True
        receipt['exe_sha256']=hashlib.sha256(args.exe.read_bytes()).hexdigest()
        (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print('PASS: native matched reimport, painted weights, materials, history, reopen and practice')


if __name__=='__main__':main()
