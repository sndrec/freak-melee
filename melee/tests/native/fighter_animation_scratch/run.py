"""Actual native scratch creation/selection/track/unload no-access proof."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent


def body(source,name):
    match=re.search(r'^[\w \t*]+\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',source,re.M)
    if not match:raise RuntimeError('source body missing: '+name)
    end=source.index('{',match.start())+1;depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[match.start():end]


def main():
    p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--expect-legacy-buffers',action='store_true')
    a=p.parse_args();out=a.output.resolve()
    if out.exists() or out.drive.upper()!='A:':p.error('fresh A: output required')
    out.mkdir(parents=True);compiler=a.compiler.resolve()/'clang.exe'
    extracted={};sources=[]
    for relative,target,names in [
        ('src/melee/ft/ftdata.c','selectors.inc',['ftData_80085B10','ftData_80085B98','ftData_80085FD4','ftData_80085CD8','ftData_80085E50']),
        ('src/melee/ft/ftanim_tracks.c','tracks.inc',['ftAnim_8006F4C8']),
        ('src/melee/ft/fighter.c','unload.inc',['Fighter_Unload_8006DABC'])]:
        path=ROOT/relative;sources.append(path);text=path.read_text();values=[]
        for name in names:
            value=body(text,name);values.append(value);extracted[name]=hashlib.sha256(value.encode()).hexdigest()
        (out/target).write_text('\n\n'.join(values)+'\n',encoding='utf-8')
    linked=[ROOT/'src/melee/gm/native_match_pool.c',ROOT/'src/melee/gm/native_match_snapshot_pool.c',ROOT/'src/melee/ft/native_fighter_pool.c',ROOT/'src/melee/gm/native_match_arena.c',ROOT/'src/melee/gm/native_match_scene.c']
    inputs=[HERE/'test.c',HERE/'README.md',Path(__file__).resolve(),compiler]+sources+linked+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
    sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    before={str(path):sha(path) for path in inputs}
    command=[str(compiler),'-std=gnu11','-O2','-DMELEE_NATIVE=1','-DEXPECT_LEGACY_BUFFERS='+str(int(a.expect_legacy_buffers)),
        '-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),str(HERE/'test.c'),
        str(ROOT/'src/melee/gm/native_match_pool.c'),str(ROOT/'src/melee/gm/native_match_snapshot_pool.c'),str(ROOT/'src/melee/ft/native_fighter_pool.c'),str(ROOT/'src/melee/gm/native_match_arena.c'),str(ROOT/'src/melee/gm/native_match_scene.c'),
        '-o',str(out/'test.exe')]
    commands=[]
    preprocess=command[:command.index(str(HERE/'test.c'))]+[str(HERE/'test.c'),'-E','-o',str(out/'preprocessed.c')]
    for stage,cmd in [('preprocess',preprocess),('compile',command),('fixture',[str(out/'test.exe')])]:
        result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
        commands.append({'stage':stage,'command':cmd,'exit':result.returncode});(out/(stage+'.log')).write_text(result.stdout+result.stderr,encoding='utf-8')
        (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n',encoding='utf-8')
        if result.returncode:raise RuntimeError(stage+' failed: '+(result.stdout+result.stderr)[-6000:])
    summary=json.loads(result.stdout)
    preprocessed=(out/'preprocessed.c').read_text()
    if not a.expect_legacy_buffers and 'fighter_x59C_alloc_data' in preprocessed:
        raise RuntimeError('legacy scratch allocator remains in native compilation')
    if any(sha(Path(path))!=expected for path,expected in before.items()):raise RuntimeError('inputs changed')
    receipt={'status':'PASS','summary':summary,'commands':commands,'input_sha256':before,'body_sha256':extracted,
        'native_legacy_allocator_present':'fighter_x59C_alloc_data' in preprocessed,
        'scope':'no native read/write of legacy archive buffers; resident selection and real interpolation dependencies distinct',
        'boundaries':['source creation/selector/track-loop/unload bodies unchanged; allocation/storage cleanup primitives controlled',
                      'resident descriptors are fixture-provided; animation attachment primitive records arguments, no real FObj decode'],
        'limits':['not full Fighter snapshot/animation rendering acceptance','negative motion indices excluded as undefined source domain']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8');print(json.dumps({'receipt':str(out/'receipt.json'),'summary':summary}))


if __name__=='__main__':main()
