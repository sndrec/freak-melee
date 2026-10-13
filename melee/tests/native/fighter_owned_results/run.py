"""Actual multi-table decoder + result admission/immutable member creation bodies."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def body(text,name):
    m=re.search(r'^(?:bool|void)\s+'+re.escape(name)+r'\([^;]*?\)\s*\{',text,re.M)
    assert m,name
    start=text.index('{',m.start()); depth=1; at=start+1
    while depth:
        depth += (text[at]=='{')-(text[at]=='}');at+=1
    return text[m.start():at]
def clip(name,frames):
    data=bytearray(32)
    struct.pack_into('>IIfII',data,0,1,0,frames,24,28)
    data[24]=255
    tail=struct.pack('>IIII',12,16,0,0)+name.encode()+b'\0'
    return struct.pack('>8I',32+len(data)+len(tail),len(data),2,1,0,0,0,0)+data+tail

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--snapshot',type=Path,default=ROOT/'.local/baseline72')
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
    sdk=a.snapshot.resolve()/'sdk';compiler=a.snapshot.resolve()/'compiler/clang.exe'
    engine=ROOT/'src/melee/ft';player=ROOT/'src/melee/pl/player.c'
    demo=(engine/'ftdemo.c').read_text(encoding='utf-8')
    source=player.read_text(encoding='utf-8')
    reset=(engine/'ftdata.c').read_text(encoding='utf-8')
    previous=(sdk/'src/melee/ft/ftdata.c').read_text(encoding='utf-8')
    (out/'bodies.inc').write_text(body(demo,'ftDemo_HasResultClips')+'\n'+body(source,'Player_CreateDemoFighters')+'\n'+body(reset,'ft_800852B0')+'\n'+body(previous,'ft_800852B0').replace('ft_800852B0','ft_800852B0_before',1)+'\n',encoding='utf-8')
    match=re.search(r'typedef struct _ftMapping\s*\{.*?\}\s*ftMapping;',source,re.S);assert match
    state=re.search(r'typedef struct ft_8045993C_t\s*\{.*?\}\s*ft_8045993C_t;',reset,re.S);assert state
    (out/'mapping.inc').write_text(match[0]+'\n'+state[0]+'\n',encoding='utf-8')
    bank=b''
    for name,frames in [('Action',10),('Win1',20),('Win1Wait',30)]:
        bank+=b'\xff'*((-len(bank))%32)+clip(name,frames)
    (out/'bank.dat').write_bytes(bank)
    overlay=out/'overlay/melee/ft';overlay.mkdir(parents=True)
    for name in ['ftanimation_bank.h','ftdemo.h']:(overlay/name).write_bytes((engine/name).read_bytes())
    units=[HERE/'test.c',engine/'ftanimation_bank.c',sdk/'src/melee/lb/lbanim_dat.c',sdk/'src/sysdolphin/baselib/dat.c']
    inputs=units+[HERE/'run.py',player,engine/'ftdemo.c',engine/'ftdata.c',engine/'ftanimation_bank.h',engine/'ftdemo.h',a.snapshot.resolve()/'freeze.json']
    inputs += [x for x in sdk.rglob('*') if x.is_file()]+[x for x in compiler.parent.rglob('*') if x.is_file()]
    manifest={str(x):sha(x) for x in sorted(set(inputs))}
    flags=['-std=gnu11','-O2','-Wall','-fms-runtime-lib=dll','-DMELEE_NATIVE=1','-D_DLL','-D_MT','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(out),'-I'+str(overlay.parent.parent),'-I'+str(sdk/'src/melee/ft'),'-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include')]
    lines=['@echo off','call "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" >nul'];objects=[];commands=[]
    for i,unit in enumerate(units):
        obj=out/f'unit{i}.obj';objects.append(obj);cmd=[compiler,*flags,'-c',unit,'-o',obj];commands.append(list(map(str,cmd)))
        lines += [subprocess.list2cmdline(list(map(str,cmd))),'if errorlevel 1 exit /b %errorlevel%']
    cmd=[compiler,*objects,'-fuse-ld=lld-link','-o',out/'test.exe'];commands.append(list(map(str,cmd)));lines += [subprocess.list2cmdline(list(map(str,cmd)))]
    (out/'compile.cmd').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    receipt={'status':'PENDING','inputs':manifest,'commands':commands,'scope':'actual decoder; extracted clip admission and player member creation; controlled ftDemo_CreateFighter allocator/render boundary'}
    def run(cmd,log):
        q=subprocess.run(list(map(str,cmd)),cwd=ROOT,capture_output=True,text=True,encoding='utf-8',errors='replace',creationflags=subprocess.CREATE_NO_WINDOW)
        (out/log).write_text(q.stdout+q.stderr,encoding='utf-8')
        if q.returncode:raise RuntimeError(q.stdout+q.stderr)
        return q.stdout
    try:
        run(['cmd','/c',out/'compile.cmd'],'compile.log')
        receipt['stdout']=run([out/'test.exe',out/'bank.dat'],'test.log');receipt['status']='PASS'
    except Exception as e:receipt['status']='FAIL';receipt['error']=str(e)
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8');print(receipt['status'],receipt.get('stdout',receipt.get('error','')))
    if receipt['status']!='PASS':raise SystemExit(1)
if __name__=='__main__':main()
