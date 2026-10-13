"""Native materializer + actual frozen importer; Python only extracts fixture inputs."""
import argparse
import hashlib
import json
import re
from pathlib import Path
import struct
import subprocess

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_archive(iso,name):
    with iso.open('rb') as f:
        header=f.read(0x440)
        assert header[:6]==b'GALE01' and header[7]==2
        at,size=struct.unpack_from('>II',header,0x424);f.seek(at);fst=f.read(size)
        count=struct.unpack_from('>I',fst,8)[0];names=fst[count*12:]
        for i in range(1,count):
            word,offset,length=struct.unpack_from('>III',fst,i*12)
            if word>>24:continue
            start=word&0xffffff
            if names[start:names.index(0,start)].decode('ascii')==name:
                f.seek(offset);data=f.read(length);assert len(data)==length;return data
    raise ValueError(name)


def body(text,name):
    m=re.search(r"^(?:static (?:inline )?)?[\w* ]+\b"+re.escape(name)+r"\([^;]*?\)\s*\{",text,re.M)
    assert m,name
    at=text.index("{",m.start());depth=1;end=at+1
    while depth:
        depth+=(text[end]=="{")-(text[end]=="}");end+=1
    return text[m.start():end]


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--iso',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();snapshot=a.snapshot.resolve();sdk=snapshot/'sdk';out=a.output.resolve()
    if out.drive.upper()!='A:' or out.exists():p.error('Fresh A: output required')
    freeze=json.loads((snapshot/'freeze.json').read_text(encoding='utf-8'))
    for name,expected in freeze.items():assert sha(snapshot/name)==expected,name
    units=[ROOT/'src/melee/ft/native_fighter_animation_copy.c',HERE/'test.c',HERE/'retarget.c',HERE/'capacity.c']
    units+=[sdk/'src/melee/ft'/name for name in ['ftcharacter_assets.c','ftdat.c','ftanimation_bank.c','ftaction_data.c','ftcostume_assets.c','ftparts_table.c']]
    units+=[sdk/'src/melee/it'/name for name in ['itarticle_data.c','itcommand_data.c','article_model_layers.c']]
    units+=[sdk/'src/melee/lb/lbanim_dat.c',sdk/'src/sysdolphin/baselib/fobj.c',sdk/'src/sysdolphin/baselib/spline.c']
    units+=[sdk/'src/sysdolphin/baselib'/name for name in ['dat.c','dat_import.c','dat_model.c','dat_geometry.c','dat_material.c','dat_animation.c','dat_scene.c','dat_texture.c']]
    inputs={str(x):sha(x) for x in units+[HERE/'run.py',ROOT/'src/melee/ft/native_fighter_animation_copy.h']}
    clang=snapshot/'compiler/clang.exe';inputs[str(clang)]=sha(clang)
    out.mkdir(parents=True)
    overlay=out/'overlay/melee/ft';overlay.mkdir(parents=True)
    (overlay/'native_fighter_animation_copy.h').write_bytes((ROOT/'src/melee/ft/native_fighter_animation_copy.h').read_bytes())
    archives=['PlNn.dat','PlNnAJ.dat','PlPp.dat','PlPpAJ.dat','PlCo.dat','PlNnNr.dat','PlNnYe.dat','PlNnAq.dat','PlNnWh.dat']
    for name in archives:(out/name).write_bytes(read_archive(a.iso,name))
    extraction={'src/melee/ft/ftanim.c':['get_part_joint','ftAnim_8006F628','ftAnim_8006F7C8','ftAnim_8006F954'],
        'src/melee/ft/ftanim_tracks.c':['ftAnim_8006F4C8','ftAnim_8006FCE4','ftAnim_8006FE08'],
        'src/melee/lb/lbanim.c':['lbAnim_InitFrames','fn_8001E60C','lbAnim_JObjSortAnim','lbAnim_8001E6D8','lbAnim_8001E7E8'],
        'src/melee/ft/ftparts.c':['ftParts_80074E58']}
    extracts=[]
    for rel,names in extraction.items():
        source=sdk/rel;inputs[str(source)]=sha(source)
        text=source.read_text(encoding='utf-8');extracts += [body(text,n) for n in names]
    (out/'retarget_bodies.h').write_text('// Exact frozen production bodies; controlled allocators/presentation only.\n'+'\n\n'.join(extracts)+'\n',encoding='utf-8',newline='\n')
    cmd=[clang,'-std=gnu11','-O2','-ffp-contract=off','-ffunction-sections','-fuse-ld=lld',
         '-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS',
         '-I'+str(out),'-I'+str(out/'overlay'),'-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include'),
         *units,'-Wl,/OPT:REF','-o',out/'copy.exe']
    batch=out/'compile.cmd'
    batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" -vcvars_ver=14.44 >nul\nif errorlevel 1 exit /b 1\n'+subprocess.list2cmdline(list(map(str,cmd)))+'\n',encoding='utf-8',newline='\n')
    stages=[('compile',['cmd','/d','/c',batch]),('test',[out/'copy.exe',*[out/n for n in archives],out/'owned-fighter.dat',out/'owned-animation.dat'])]
    commands=[]
    for stage,command in stages:
        command=list(map(str,command))
        r=subprocess.run(command,cwd=ROOT,text=True,encoding='utf-8',capture_output=True,timeout=300,creationflags=subprocess.CREATE_NO_WINDOW)
        (out/(stage+'.log')).write_text(r.stdout+r.stderr,encoding='utf-8',newline='\n')
        commands.append({'stage':stage,'command':command,'exit':r.returncode})
        if r.returncode:raise RuntimeError(stage+'\n'+r.stdout+r.stderr)
    for name,expected in inputs.items():assert sha(Path(name))==expected,name
    for name,expected in freeze.items():assert sha(snapshot/name)==expected,name
    receipt={'status':'PASS native authoring materializer and actual frozen fighter/animation imports',
        'result':dict(json.loads(r.stdout),retarget=dict(zip(['pairs','samples','checks'],map(int,re.search(r'retarget (\d+) pairs / (\d+) scalar pose samples / (\d+) checks',r.stderr).groups()))),capacity=dict(zip(['cases','checks'],map(int,re.search(r'capacity (\d+) cases / (\d+) checks',r.stderr).groups())))),'snapshot_sha256':sha(snapshot/'freeze.json'),
        'source_overlay':'Only native_fighter_animation_copy.c/h are live candidate units; all importer sources and SDK headers frozen.',
        'inputs':inputs,'commands':commands,'archives':{n:sha(out/n) for n in archives},
        'extracted_bodies_sha256':sha(out/'retarget_bodies.h'),'outputs':{n:sha(out/n) for n in ['owned-fighter.dat','owned-animation.dat']}}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(receipt['result']))


if __name__=='__main__':main()
