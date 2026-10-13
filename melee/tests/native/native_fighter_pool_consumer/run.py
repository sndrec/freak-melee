"""Actual Fighter normal/demo constructor, satellites, unload and snapshot fixture."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent


def body(text,name):
    found=re.search(r'^[\w \t*]+\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',text,re.M)
    if not found:raise RuntimeError('source body missing: '+name)
    end=text.index('{',found.start())+1;depth=1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[found.start():end]


def main():
    p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();out=a.output.resolve()
    if out.exists() or out.drive.upper()!='A:':p.error('fresh A: output required')
    out.mkdir(parents=True);compiler=a.compiler.resolve()/'clang.exe'
    originals=[];extracted={}
    for relative,target,names in [
        ('fighter.c','family.inc',['Fighter_800679B0','Fighter_FirstInitialize_80067A84']),
        ('fighter.c','create.inc',['Fighter_Create_Inline2','Fighter_Create']),
        ('fighter.c','unload.inc',['Fighter_Unload_8006DABC']),
        ('ftdemo.c','demo.inc',['initFighter','ftDemo_CreateFighter']),
        ('ftparts.c','parts.inc',['ftParts_80074E58'])]:
        path=ROOT/'src/melee/ft'/relative;originals.append(path);text=path.read_text();values=[]
        for name in names:
            value=body(text,name);values.append(value);extracted[name]=hashlib.sha256(value.encode()).hexdigest()
        if target=='family.inc':values.insert(0,'#define g_spawnNumCounter Fighter_804D64F8')
        if target=='parts.inc':
            path=ROOT/'src/melee/ft/ft_0C88.c';originals.append(path);value=body(path.read_text(),'ftCo_800C884C');values.append(value)
            extracted['ftCo_800C884C']=hashlib.sha256(value.encode()).hexdigest()
        (out/target).write_text('\n\n'.join(values)+'\n',encoding='utf-8')
    sources=[HERE/'test.c',ROOT/'src/melee/gm/native_match_pool.c',ROOT/'src/melee/gm/native_match_snapshot_pool.c',ROOT/'src/melee/gm/native_rollback_state.c',ROOT/'src/melee/ft/native_fighter_pool.c']
    sources += [ROOT/'src/sysdolphin/baselib'/name for name in ['gobj.c','gobjinit.c','gobjplink.c','gobjproc.c','gobjgxlink.c','gobjuserdata.c','gobjobject.c','native_gobj_pool_storage.c']]
    sources += [ROOT/'src/melee/gr/native_ground_pool.c',ROOT/'src/melee/it/native_item_pool.c',ROOT/'src/melee/it/native_item_link_pool.c']
    sources += [ROOT/'tests/native/item_actor_absent.c']
    inputs=sources+originals+[compiler,Path(__file__).resolve()]+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
    sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest();before={str(path):sha(path) for path in inputs}
    command=[str(compiler),'-std=gnu11','-O2','-ffunction-sections','-fdata-sections','-fno-zero-initialized-in-bss','-fuse-ld=lld','-Wl,/opt:ref',
        '-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),
        '-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out)]+list(map(str,sources))+['-o',str(out/'test.exe')]
    commands=[]
    for stage,cmd in [('compile',command),('fixture',[str(out/'test.exe')])]:
        result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
        commands.append({'stage':stage,'command':cmd,'exit':result.returncode});(out/(stage+'.log')).write_text(result.stdout+result.stderr,encoding='utf-8')
        (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n',encoding='utf-8')
        if result.returncode:raise RuntimeError(stage+' failed: '+(result.stdout+result.stderr)[-6000:])
    summary=json.loads(result.stdout)
    if any(sha(Path(path))!=expected for path,expected in before.items()):raise RuntimeError('input changed')
    receipt={'status':'PASS','summary':summary,'commands':commands,'input_sha256':before,'body_sha256':extracted,
        'boundaries':['complete actual normal/demo creation, parts/list assignment, family init and unload bodies extracted unchanged',
                      'actual HSD GObj/proc lifecycle, scene-owned pools and PE .mstate recognition',
                      'asset/graph/camera/script/CPU primitives controlled; remaining owner state requires page oracle'],
        'limits':['not complete explicit game snapshot, script state or rendering acceptance','64 actor capacity provisional; shared Kirby/Purin extra-list allocator remains genuine owner']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8');print(json.dumps({'receipt':str(out/'receipt.json'),'summary':summary}))


if __name__=='__main__':main()
