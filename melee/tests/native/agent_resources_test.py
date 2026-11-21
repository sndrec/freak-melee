"""Exercise owned resource transactions using the installed native tool only."""
import ctypes
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zlib


def snapshot(root):
    return {p.relative_to(root).as_posix():hashlib.sha256(p.read_bytes()).hexdigest()
            for p in root.rglob('*') if p.is_file()}


def png(color):
    def chunk(tag,data):
        return struct.pack('>I',len(data))+tag+data+struct.pack('>I',zlib.crc32(tag+data))
    return (b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',8,8,8,6,0,0,0))+
            chunk(b'IDAT',zlib.compress((b'\0'+bytes(color)*8)*8))+chunk(b'IEND',b''))


def common_item(iso,kind):
    # Independent ISO/FST and DAT table walk: no tool-produced expected bytes.
    with iso.open('rb') as stream:
        stream.seek(0x424);offset,size=struct.unpack('>II',stream.read(8));stream.seek(offset);fst=stream.read(size)
        count=struct.unpack_from('>I',fst,8)[0];strings=fst[count*12:]
        for i in range(1,count):
            tag,offset,size=struct.unpack_from('>III',fst,i*12);name=tag&0xffffff
            if strings[name:strings.find(b'\0',name)]==b'ItCo.dat':
                stream.seek(offset);data=stream.read(size);break
        else:raise AssertionError('No ItCo.dat in test ISO')
    size,relocations,roots,externs=struct.unpack_from('>IIII',data,4)
    root_at=32+size+relocations*4;strings_at=root_at+(roots+externs)*8
    for i in range(roots):
        offset,name=struct.unpack_from('>II',data,root_at+i*8);at=strings_at+name
        if data[at:data.find(b'\0',at)]==b'itPublicData':
            table=struct.unpack_from('>I',data,32+offset+4)[0]
            return data,struct.unpack_from('>I',data,32+table+kind*4)[0]
    raise AssertionError('No itPublicData root')


def resource_checks(call,target,project,fixtures,*,live=False):
    fixtures.mkdir(exist_ok=True)
    image=fixtures/'token.png';image.write_bytes(png((200,80,20,255)))
    other=fixtures/'other.png';other.write_bytes(png((20,120,240,255)))
    blob=fixtures/'data.bin';blob.write_bytes(b'\0\xffowned binary data\0')
    before_disk=snapshot(project)
    prefix='session.' if live else ''
    read=prefix+'resources.read';edit=prefix+'resources.edit';export=prefix+'resources.export'
    original=call(read,target)
    assert original['limits']['entries']==64 and original['limits']['blob_bytes']==4*1024*1024
    assert {row['category'] for row in original['asset_formats']}=={'textures','items','articles','sounds','effects'}
    assert len(original['common_items'])==43 and original['common_items'][0]==dict(stock_kind=0,name='Capsule')
    assert any(row['declaration']=='It_Kind_Mario_Fire' and row['slot']==0 for row in original['articles']),original['articles']
    def rows():return call(read,dict(target,limit=200))['resources']
    def change(changes):
        args=dict(target,revision=call(read,target)['revision'],changes=changes)
        if live:args['summary']='Resource authoring check'
        return call(edit,args)
    def reject(changes):
        old=call(read,dict(target,limit=200));disk=snapshot(project)
        args=dict(target,revision=old['revision'],changes=changes)
        if live:args['summary']='Reject resource batch'
        error=call(edit,args,expected=False)
        current=call(read,dict(target,limit=200))
        assert current['revision']==old['revision'] and current['resources']==old['resources']
        assert snapshot(project)==disk
        return error
    imports=[dict(operation='import',name='agent/token.png',kind='png',path=str(image)),
             dict(operation='import',name='agent/data.bin',kind='blob',path=str(blob))]
    first=change(imports)
    assert first['changed'] and first['saved']!=live and not first['installed']
    after=rows();names={row['name']:row for row in after}
    assert names['agent/token.png']['width']==8 and names['agent/token.png']['height']==8
    assert names['agent/data.bin']['sha256']==hashlib.sha256(blob.read_bytes()).hexdigest()
    same=change(imports);assert not same['changed'] and same['revision']==first['revision']
    assert call(read,dict(target,query='AGENT/',limit=1))['next_offset']==1
    assert call(read,dict(target,query='agent/',offset=1,limit=1))['next_offset'] is None
    args=dict(target,revision=original['revision'],changes=imports)
    if live:args['summary']='Stale resource edit'
    assert call(edit,args,expected=False)['code']=='revision_conflict'
    output=fixtures/'export-雪.png';exported=call(export,dict(target,name='agent/token.png',output=str(output)))
    assert output.read_bytes()==image.read_bytes() and exported['sha256']==names['agent/token.png']['sha256']
    call(export,dict(target,name='agent/token.png',output=str(output)),expected=False)
    assert output.read_bytes()==image.read_bytes()
    for invalid in ('../outside','items/CON.png','AGENT/token.png'):
        reject([dict(operation='import',name=invalid,kind='png',path=str(image))])
    reject([imports[0],imports[0]])
    reject([dict(operation='import',name='agent/token.png',kind='png',path=str(other)),
            dict(operation='import',name='agent/missing',kind='blob',path=str(fixtures/'absent'))])
    reject([dict(operation='remove',name='agent/data.bin'),dict(operation='remove',name='absent')])
    reject([dict(operation='remove',name='agent/data.bin',path=str(blob))])
    bad=fixtures/'bad.png';bad.write_bytes(image.read_bytes()[:-1]+b'\xff')
    reject([dict(operation='import',name='agent/bad.png',kind='png',path=str(bad))])
    huge=fixtures/'huge.bin';huge.write_bytes(bytes(4*1024*1024+1))
    reject([dict(operation='import',name='agent/huge',kind='blob',path=str(huge))])
    chunk=fixtures/'chunk.bin';chunk.write_bytes(bytes(3*1024*1024))
    batch=[dict(operation='import',name=f'agent/chunk{i}',kind='blob',path=str(chunk)) for i in range(3)]
    assert reject(batch)['code']=='resource_input_unavailable'
    malformed=fixtures/'bad.item';malformed.write_bytes(b'FMIB'+bytes(40))
    reject([dict(operation='import',name='agent/bad.item',kind='blob',path=str(malformed))])
    if live:
        assert snapshot(project)==before_disk
        state=call('session.inspect',target);assert state['dirty'] and state['can_undo']
        undone=call('session.undo',dict(target,revision=state['revision']))
        assert rows()==original['resources'] and not undone['dirty']
        redone=call('session.redo',dict(target,revision=undone['revision']))
        assert rows()==after and redone['dirty']
        # Resource edits share Model history too, and survive Save then Undo/Redo.
        state=call('session.workspace.select',dict(target,revision=redone['revision'],workspace='model'))
        changed=change([dict(operation='import',name='agent/token.png',kind='png',path=str(other))])
        undone=call('session.undo',dict(target,revision=changed['revision']))
        assert next(row for row in rows() if row['name']=='agent/token.png')['sha256']==hashlib.sha256(image.read_bytes()).hexdigest()
        changed=call('session.redo',dict(target,revision=undone['revision']))
        assert snapshot(project)==before_disk
        saved=call('session.save',dict(target,revision=changed['revision']))
        assert not saved['dirty'] and saved['saved']
        undone=call('session.undo',dict(target,revision=saved['revision']));assert undone['dirty']
        changed=call('session.redo',dict(target,revision=undone['revision']));assert not changed['dirty']
        baseline=snapshot(project)
        assert call('resources.edit',dict(project=str(project),revision=call('resources.read',dict(project=str(project)))['revision'],changes=imports),expected=False)['code']=='draft_busy'
        assert snapshot(project)==baseline
    else:
        changed_files={name for name,value in snapshot(project).items() if before_disk.get(name)!=value}
        assert changed_files=={'resources.fmr'},changed_files
    removed=change([dict(operation='remove',name='agent/data.bin'),dict(operation='remove',name='agent/token.png')])
    assert rows()==original['resources']
    if not live:assert snapshot(project)==before_disk
    return dict(atomic_resources=True,resource_export=True,resource_history=live)


def main():
    tool,iso=(Path(value).resolve() for value in sys.argv[1:3])
    windows=Path(os.environ['SystemRoot'])
    with tempfile.TemporaryDirectory(prefix='Native resource agent ') as temporary:
        root=Path(temporary);profile=root/'userdata';profile.mkdir()
        env=dict(os.environ,LOCALAPPDATA=str(profile),PATH=str(windows/'System32')+';'+str(windows))
        def call(name,args=None,expected=True):
            result=subprocess.run([str(tool),'agent','call',name,'--stdin'],input=json.dumps(args or {}),
                env=env,cwd=root,capture_output=True,encoding='utf-8',timeout=90,creationflags=subprocess.CREATE_NO_WINDOW)
            assert result.stdout,(name,result.returncode,result.stderr)
            reply=json.loads(result.stdout);assert reply['ok']==expected,(name,args,reply)
            return reply['result'] if expected else reply['error']
        mario=next(row for row in call('stock.list',dict(query='Mario'))['fighters'] if row['name']=='Mario')
        project=root/'Mario'
        call('project.clone_stock',dict(iso=str(iso),fighter_id=mario['fighter_id'],title='Resources',output=str(project)))
        result=resource_checks(call,dict(project=str(project)),project,root/'inputs')
        # The final collection fits even when new names precede removals in the edit.
        state=call('resources.read',dict(project=str(project)))
        chunk=root/'inputs/chunk.bin'
        def chunk_edit(name):
            return dict(operation='import',name=name,kind='blob',path=str(chunk))
        initial_imports=[chunk_edit('old/one'),chunk_edit('old/two')]
        initial=call('resources.edit',dict(project=str(project),revision=state['revision'],changes=initial_imports))
        replacements=[chunk_edit('new/one'),chunk_edit('new/two')]
        removals=[dict(operation='remove',name=row['name']) for row in initial_imports]
        replaced=call('resources.edit',dict(project=str(project),revision=initial['revision'],changes=replacements+removals))
        assert replaced['bundle_bytes']==initial['bundle_bytes']
        call('resources.edit',dict(project=str(project),revision=replaced['revision'],
                                   changes=[dict(operation='remove',name=row['name']) for row in replacements]))
        # Failed publication under a real Windows file lock preserves the bundle.
        state=call('resources.read',dict(project=str(project)));before=snapshot(project)
        bundle=project/'resources.fmr'
        kernel=ctypes.WinDLL('kernel32',use_last_error=True)
        kernel.CreateFileW.argtypes=[ctypes.c_wchar_p,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_void_p,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_void_p]
        kernel.CreateFileW.restype=ctypes.c_void_p;kernel.CloseHandle.argtypes=[ctypes.c_void_p]
        # Seed an entry so a later no-op can acquire the lease and recover a
        # deferred rollback once the external Windows lock has been released.
        blob=root/'inputs/data.bin'
        seed=[dict(operation='import',name='lock/base',kind='blob',path=str(blob))]
        call('resources.edit',dict(project=str(project),revision=state['revision'],changes=seed))
        state=call('resources.read',dict(project=str(project)));before=snapshot(project);before_bytes=bundle.read_bytes()
        handle=kernel.CreateFileW(str(bundle),0x80000000,1,None,3,0x80,None)
        assert handle!=ctypes.c_void_p(-1).value
        try:
            call('resources.edit',dict(project=str(project),revision=state['revision'],changes=[dict(operation='import',name='lock/new',kind='blob',path=str(blob))]),expected=False)
        finally:assert kernel.CloseHandle(handle)
        assert bundle.read_bytes()==before_bytes
        recovered=call('resources.edit',dict(project=str(project),revision=state['revision'],changes=seed))
        assert not recovered['changed']
        assert snapshot(project)==before
        accepted=bundle.read_bytes();bundle.write_bytes(b'bad resource header')
        damaged=snapshot(project)
        call('resources.edit',dict(project=str(project),revision=state['revision'],changes=[dict(operation='import',name='bad/new',kind='blob',path=str(blob))]),expected=False)
        assert snapshot(project)==damaged;bundle.write_bytes(accepted)
        state=call('resources.read',dict(project=str(project)));kept=state['bundle_entries']
        capacity=[dict(operation='import',name='capacity/'+str(i),kind='blob',path=str(blob)) for i in range(64-kept)]
        filled=call('resources.edit',dict(project=str(project),revision=state['revision'],changes=capacity))
        assert filled['bundle_entries']==64
        call('resources.edit',dict(project=str(project),revision=filled['revision'],changes=[dict(operation='import',name='overflow',kind='blob',path=str(blob))]),expected=False)
        call('resources.edit',dict(project=str(project),revision=filled['revision'],changes=[dict(operation='remove',name=row['name']) for row in capacity]))
        blueprint=root/'capsule.item'
        built=call('resources.blueprint',dict(iso=str(iso),stock_kind=0,output=str(blueprint)))
        assert built['graph_validated'] and blueprint.read_bytes()[:4]==b'FMIB'
        data=blueprint.read_bytes();size,offset,reserved=struct.unpack_from('<III',data,4)
        assert size==built['dat_bytes'] and offset==built['article_offset'] and reserved==0
        actual_dat,actual_offset=common_item(iso,0)
        assert data[16:]==actual_dat and offset==actual_offset
        dat=root/'ItCo.dat';dat.write_bytes(data[16:]);explicit=root/'explicit.item'
        call('resources.blueprint',dict(dat=str(dat),article_offset=offset,output=str(explicit)))
        assert explicit.read_bytes()==data
        call('resources.blueprint',dict(dat=str(dat),article_offset=0xffffffff,output=str(explicit)),expected=False)
        assert explicit.read_bytes()==data
        call('resources.blueprint',dict(iso=str(iso),stock_kind=0,output=str(blueprint)),expected=False)
        assert blueprint.read_bytes()==data
        state=call('resources.read',dict(project=str(project)))
        call('resources.edit',dict(project=str(project),revision=state['revision'],changes=[
            dict(operation='import',name='agent/capsule.item',kind='blob',path=str(blueprint))]))
        assert call('resources.read',dict(project=str(project),query='capsule'))['resources'][0]['format']=='item_blueprint'
        # The original source's own item recipes can reference the new blueprint.
        sources=call('scripts.list',dict(project=str(project)))
        source=sources['files'][0]['file'];text=call('scripts.read',dict(project=str(project),file=source))['text']
        text+='\nitem AgentCapsule damage 3 angle 45 lifetime 120:\n    blueprint agent/capsule.item\n'
        call('scripts.update',dict(project=str(project),revision=sources['revision'],files=[dict(file=source,text=text)]))
        checked=call('scripts.check',dict(project=str(project)));assert checked['accepted'],checked
        # Both paired members own separate bundles; a batch must change only one.
        zelda=next(row for row in call('stock.list',dict(query='Zelda'))['fighters'] if row['name']=='Zelda')
        family=root/'Zelda';call('project.clone_stock',dict(iso=str(iso),fighter_id=zelda['fighter_id'],title='Resource family',output=str(family)))
        baseline=snapshot(family);target=dict(project=str(family),member='partner')
        state=call('resources.read',target)
        call('resources.edit',dict(target,revision=state['revision'],changes=[dict(operation='import',name='agent/token',kind='blob',path=str(blueprint))]))
        after=snapshot(family);assert {name for name in after if after[name]!=baseline.get(name)}=={'partner/resources.fmr'}
        print('PASS: owned resource batches, exact export, blueprint graph admission, script references and member isolation',result)


if __name__=='__main__':main()
