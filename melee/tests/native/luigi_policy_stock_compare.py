"""Compare actual generated Luigi callbacks and shared production policy bodies.

Runs only isolated compiler/source checks against an already prepared project.
No global build or game process is launched.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
CPU = 'src/melee/ft/kinds/ftCommon/ftCo_0A01.c'
ATTACK = 'src/melee/ft/ftcpuattack.c'
METADATA = 'src/melee/ft/ft_0892.c'


def function(source, name):
    # These production functions close at column zero. Mutually exclusive
    # preprocessor switch openings cannot be balanced as raw C tokens.
    start = re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b' +
                      re.escape(name) + r'\s*\([^;{}]*\)\s*\{', source)
    assert start, name
    end = re.search(r'(?m)^}', source[start.end():])
    assert end, name
    return source[start.start():start.end()+end.end()] + '\n'


def excerpts(batch, sdk, original):
    sources = {path:(original/path).read_text() for path in (CPU,ATTACK,METADATA)}
    text = '// Baseline11 originals; only comparison entry names isolated.\n'
    text += function(sources[CPU], 'inlineC0')
    text += function(sources[CPU], 'ftCo_800A3498').replace('ftCo_800A3498','original_destination')
    text += function((sdk/CPU).read_text(),'ftCo_800A3498').replace('ftCo_800A3498','production_destination')
    for path, name, following, signature in (
        (ATTACK,'ftCo_800B77E8','Ft_Kind_Peach','Fighter* target; int can_attack;'),
        (CPU,'ftCo_800A96B8','Ft_Kind_Zelda','')):
        whole=function(sources[path],name)
        begin=whole.index('    case Ft_Kind_Luigi:')
        end=whole.index('    case '+following+':',begin)
        entry='original_ranged' if path==ATTACK else 'original_recovery'
        text+='static void '+entry+'(Fighter* fp) { '+signature+' switch(fp->kind) {\n'+whole[begin:end]+'default: break; } }\n'
    text += function((sdk/ATTACK).read_text(),'ftCo_CpuAddAttackCandidate')
    source=(sdk/METADATA).read_text()
    typedef=re.search(r'typedef struct \{.*?\} UnkStruct89768;',source,re.S)
    assert typedef
    text+=typedef[0]+'\n'+function(source,'ft_80089768')
    text+=function(source,'decode_action_metadata')
    text+=function(source,'ft_800895E0')
    text+=function(sources[METADATA],'ft_800895E0').replace('ft_800895E0','malformed_native_original')
    (batch/'luigi_policy_original.inc').write_text(text)
    clone=(sdk/'src/melee/ft/kinds/ftLuigi/ftluigi_clone.c').read_text()
    rig=re.search(r'static const NativeFighterBone ftLuigiClone_Bones\[\] = \{.*?\n\};',clone,re.S)
    assert rig,'Owned Luigi rig missing from matching SDK'
    (batch/'luigi_policy_rig.inc').write_text(rig[0]+'\n')
    return {str(base/path):hashlib.sha256((base/path).read_bytes()).hexdigest()
            for base in (original,sdk) for path in (CPU,ATTACK,METADATA)}


def unused_host(batch,sdk,project):
    required=set(re.findall(r'host->(\w+) == 0',(project/'behavior.generated.c').read_text()))
    provided={'callback_source_alive','cpu_command','cpu_command_arg','cpu_ranged_unsafe','cpu_add_candidate'}
    header=(sdk/'src/melee/ft/native_fighter_module.h').read_text().split('typedef struct NativeFighterHost {',1)[1].split('} NativeFighterHost;',1)[0]
    signatures={m[2]:(m[1].strip(),m[3].strip()) for m in re.finditer(r'^\s*([^;\n]+?)\(\*(\w+)\)\((.*?)\);',header,re.M|re.S)}
    for name in re.findall(r'^\s*HSD_GObjEvent (\w+);',header,re.M):signatures[name]=('void','HSD_GObj*')
    code,assign=[],[]
    for name in sorted(required-provided):
        result,args=signatures[name];parts=[];depth=0;begin=0
        for at,ch in enumerate(args):
            depth+=(ch=='(')-(ch==')')
            if ch==',' and depth==0:parts.append(args[begin:at].strip());begin=at+1
        parts.append(args[begin:].strip())
        named=[]
        for i,arg in enumerate(parts):
            if arg in ('void','...'):named.append(arg)
            elif '(*)' in arg:named.append(arg.replace('(*)','(*unused_'+str(i)+')'))
            else:
                # A few actual Host declarations name their parameters.
                if re.search(r'\s+[A-Za-z_]\w*$',arg) and arg.split()[-1] not in ('int','long','short','char','float','double','bool'):
                    arg=arg.rsplit(None,1)[0]
                named.append(arg+' unused_'+str(i))
        code.append('static '+result+' trap_'+name+'('+','.join(named)+') { require(false,"Unexpected Host '+name+'"); abort(); }\n')
        assign.append('host->'+name+'=trap_'+name+';')
    (batch/'luigi_policy_unused_host.inc').write_text(''.join(code)+'static void supply_unused_host(NativeFighterHost* host) {\n'+'\n'.join(assign)+'\n}\n')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--snapshot',required=True,type=Path)
    p.add_argument('--original-sdk',default=ROOT/'.local/baseline11/sdk',type=Path)
    p.add_argument('--project',required=True,type=Path)
    p.add_argument('--batch',required=True,type=Path)
    a=p.parse_args();snapshot=a.snapshot.resolve();sdk=snapshot/'sdk';project=a.project.resolve();batch=a.batch.resolve();batch.mkdir(parents=True,exist_ok=True)
    facts=excerpts(batch,sdk,a.original_sdk.resolve());unused_host(batch,sdk,project)
    flags=[str(snapshot/'compiler/clang.exe'),'-fms-runtime-lib=dll','-std=gnu11','-O2','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-D_DLL','-D_MT','-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include'),'-I'+str(batch)]
    dll=batch/'luigi_generated.dll';exe=batch/'luigi_policy_stock_test.exe'
    commands=[flags+['-shared',str(project/'behavior.generated.c'),'-o',str(dll)],flags+[str(ROOT/'tests/native/luigi_policy_stock_test.c'),str(sdk/'src/melee/ft/native_callback_lifetime.c'),str(sdk/'src/melee/ft/native_fighter_parameters.c'),str(ROOT/'tests/native/model_effect_absent_fixture.c'),'-o',str(exe)]]
    setup='C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat'
    cmd=batch/'compile.cmd';cmd.write_text('@echo off\ncall '+subprocess.list2cmdline([setup])+' >nul\n'+'\nif errorlevel 1 exit /b 1\n'.join(subprocess.list2cmdline(c) for c in commands)+'\n')
    with (batch/'build.log').open('wb') as log:r=subprocess.run(['cmd','/c',str(cmd)],cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
    if r.returncode:print((batch/'build.log').read_text());return r.returncode
    r=subprocess.run([str(exe),str(dll),str(project/'fighter_parameters.fmp')],cwd=ROOT,text=True,capture_output=True)
    output=r.stdout+r.stderr;(batch/'result.log').write_text(output)
    paths=[dll,project/'behavior.fms',project/'behavior.generated.c',project/'fighter_parameters.fmp',ROOT/'tests/native/luigi_policy_stock_compare.py',ROOT/'tests/native/luigi_policy_stock_test.c',batch/'luigi_policy_original.inc',snapshot/'freeze.json']
    (batch/'result.json').write_text(json.dumps(dict(exit_code=r.returncode,output=output,snapshot=str(snapshot),source_sha256=facts,sha256={str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in paths}),indent=2)+'\n')
    print(output,end='');return r.returncode


if __name__=='__main__':raise SystemExit(main())
