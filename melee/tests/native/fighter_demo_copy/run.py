"""Retail demo materialization, immutable support SDK, actual current copier/importer."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--baseline', type=Path, required=True)
    p.add_argument('--iso', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    out = a.output.resolve()
    if out.drive.upper() != 'A:' or out.exists():
        p.error('Fresh isolated A: output required')
    out.mkdir(parents=True)
    sdk = a.baseline.resolve() / 'sdk'
    sys.path.insert(0, str(ROOT / 'tools'))
    from fighter_animation_io import disc_file
    mappings = {
        'Mr':'ftMario/ftmario_clone.c', 'Fx':'ftFox/ftfox_clone.c',
        'Ca':'ftCaptain/ftcaptain_clone.c', 'Dk':'ftDonkey/ftdonkey_clone.c',
        'Kp':'ftKoopa/ftkoopa_clone.c', 'Lk':'ftLink/ftlink_clone.c',
        'Sk':'ftSeak/ftsheik_clone.c', 'Ns':'ftNess/ftness_clone.c',
        'Pe':'ftPeach/ftpeach_clone.c', 'Pp':'ftPopo/ftpopo_clone.c',
        'Nn':'ftNana/ftnana_clone.c', 'Pk':'ftPikachu/ftpikachu_clone.c',
        'Ss':'ftSamus/ftsamus_clone.c', 'Ys':'ftYoshi/ftyoshi_clone.c',
        'Pr':'ftPurin/ftpurin_clone.c', 'Mt':'ftMewtwo/ftmewtwo_clone.c',
        'Lg':'ftLuigi/ftluigi_clone.c', 'Ms':'ftMars/ftmars_clone.c',
        'Zd':'ftZelda/ftzelda_clone.c', 'Cl':'ftCLink/ftclink_clone.c',
        'Dr':'ftDrMario/ftdrmario_clone.c', 'Fc':'ftFalco/ftfalco_clone.c',
        'Pc':'ftPichu/ftpichu_clone.c', 'Gw':'ftGameWatch/ftgamewatch_clone.c',
        'Gn':'ftGanon/ftganon_clone.c', 'Fe':'ftEmblem/ftemblem_clone.c',
    }
    archives = {'IrAls.dat','GmRegEnd.dat','Vi0102.dat','Vi1101.dat'}
    rows, manifests, metadata_inputs = [], [], []
    for code, relative in mappings.items():
        path = ROOT / 'src/melee/ft/kinds' / relative
        text = path.read_text(encoding='utf-8')
        metadata_inputs.append(path)
        root = re.search(r'\.fighter_root\s*=\s*"([^"]+)"',text)[1]
        actions = int(re.search(r'\.action_count\s*=\s*(\d+)',text)[1])
        demos = int(re.search(r'\.demo_count\s*=\s*(\d+)',text)[1])
        names = re.findall(r'"(ftDemo(?:Result|Intro|Ending|ViWait)MotionFile[^\"]+)"',text)
        assert len(names) == 4, path
        result = 'GmRstM' + ('Pn' if code in ('Pp','Nn') else code) + '.dat'
        # DVD path lookup is case-insensitive (dvdfs.c). Extract exact FST
        # spelling here; vi's Marth string spells its final J lowercase.
        wait = 'Pl' + ('Pp' if code == 'Nn' else code) + 'DViWaitAJ.dat'
        if code == 'Nn': names[3] = 'ftDemoViWaitMotionFilePopo'
        extras = [f'ftDemoVi0102MotionFile{root[6:]}',f'ftDemoVi1101MotionFile{root[6:]}'] if demos == 16 else [None,None]
        fields = [code,root,actions,demos,result,*names[:3],wait,names[3],*extras]
        rows.append('    {' + ','.join('NULL' if x is None else json.dumps(x) for x in fields) + '}')
        archives.update(('Pl'+code+'.dat','Pl'+code+'AJ.dat',result,wait))
        manifests.append(dict(zip(('code','root','actions','demos','result','result_root','intro_root','ending_root','wait','wait_root','extra0','extra1'),fields)))
    (out/'cases.inc').write_text('static const Case cases[] = {\n'+',\n'.join(rows)+'\n};\n',encoding='utf-8')
    retail = {}
    for name in sorted(archives):
        data = disc_file(a.iso.resolve(), name)
        (out/name).write_bytes(data)
        retail[name] = hashlib.sha256(data).hexdigest()
    overlay = out/'overlay/melee/ft'
    overlay.mkdir(parents=True)
    current = ROOT/'src/melee/ft'
    headers = ('native_fighter_demo_copy.h','native_fighter_dat_copy.h','native_fighter_animation_copy.h','ftanimation_bank.h')
    for name in headers:
        (overlay/name).write_bytes((current/name).read_bytes())
    units = [HERE/'test.c'] + [current/(name+'.c') for name in
        ('native_fighter_demo_copy','native_fighter_animation_copy','native_fighter_dat_copy','ftanimation_bank')]
    units += [sdk/'src/sysdolphin/baselib/dat.c',sdk/'src/melee/lb/lbanim_dat.c']
    compiler = a.baseline.resolve()/'compiler/clang.exe'
    inputs = set(units + metadata_inputs + [HERE/'run.py',ROOT/'tools/fighter_animation_io.py',current/'native_fighter_demo_copy.h',current/'ftanimation_bank.h',a.baseline.resolve()/'freeze.json'])
    inputs.update(current / name for name in headers)
    inputs.add(current / 'types.h')
    inputs.update(x for x in sdk.rglob('*') if x.is_file())
    inputs.update(x for x in compiler.parent.rglob('*') if x.is_file())
    hashes = {str(x):sha(x) for x in sorted(inputs)}
    flags = ['-std=gnu11','-O2','-Wall','-fms-runtime-lib=dll','-DMELEE_NATIVE=1','-D_DLL','-D_MT','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS',
             '-I'+str(out),'-I'+str(current),'-I'+str(out/'overlay'),'-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include')]
    objects=[]
    lines=['@echo off','call "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" >nul']
    commands=[]
    for i,unit in enumerate(units):
        obj=out/f'unit{i}.obj';objects.append(obj)
        cmd=[compiler,*flags,'-c',unit,'-o',obj];commands.append(list(map(str,cmd)))
        lines += [subprocess.list2cmdline(list(map(str,cmd))), 'if errorlevel 1 exit /b %errorlevel%']
    cmd=[compiler,*objects,'-fuse-ld=lld-link','-o',out/'test.exe'];commands.append(list(map(str,cmd)))
    lines.append(subprocess.list2cmdline(list(map(str,cmd))))
    batch=out/'compile.cmd';batch.write_text('\n'.join(lines)+'\n',encoding='utf-8')
    def run(cmd,log):
        r=subprocess.run(list(map(str,cmd)),cwd=ROOT,capture_output=True,text=True,creationflags=subprocess.CREATE_NO_WINDOW)
        (out/log).write_text(r.stdout+r.stderr,encoding='utf-8')
        if r.returncode: raise RuntimeError(str(out/log)+'\n'+(r.stdout+r.stderr)[-6000:])
        return r.stdout
    run(['cmd','/c',batch],'compile.log')
    result=run([out/'test.exe',out],'run.log')
    assert 'PASS owned demos:' in result
    for path,value in hashes.items(): assert sha(Path(path))==value, 'Input changed: '+path
    receipt={'status':'PASS','summary':result.strip().splitlines()[-1],'commands':commands,'inputs':hashes,'retail':retail,'members':manifests,
             'boundaries':['Actual DAT accessor, demo copier, Nana action/article materializers and generic animation decoder.',
                           'No game, editor, results-scene, project constructor or allocator-failure injection in this console receipt. Normal constructor CLI integration is a separate gate.']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
    print(receipt['summary']);print(out/'receipt.json')


if __name__ == '__main__': main()
