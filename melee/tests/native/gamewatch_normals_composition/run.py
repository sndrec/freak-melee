"""SDK57 normal prop/lifecycle console comparison; never starts a game."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
sys.dont_write_bytecode = True
from compose import compose
HERE = Path(__file__).resolve().parent
FIGHTERS = ['ftgamewatchattack11.c','ftgamewatchattack100.c','ftgamewatchattacklw3.c',
            'ftgamewatchattacks4.c','ftgamewatchattackair.c']
ARTICLES = ['itgamewatchgreenhouse.c','itgamewatchmanhole.c','itgamewatchfire.c',
            'itgamewatchparachute.c','itgamewatchturtle.c','itgamewatchbreath.c']
NORMAL_FUNCTIONS = {
 'ftgamewatchattack11.c': ['ftGw_Attack11_DecideAction','ftGw_Attack11_ItemGreenhouseSetFlag',
    'ftGw_Attack11_ItemGreenhouseRemove','ftGw_Attack11_ItemGreenhouseEnterHitlag',
    'ftGw_Attack11_ItemGreenhouseExitHitlag','ftGw_Attack11_ItemGreenhouse_CheckAll',
    'ftGw_Attack11_ItemGreenhouse_CheckAttack11'],
 'ftgamewatchattack100.c': [],
 'ftgamewatchattacklw3.c': ['ftGw_AttackLw3_ItemManholeRemove','ftGw_AttackLw3_ItemManholeOnDamage',
    'ftGw_AttackLw3_ItemManholeEnterHitlag','ftGw_AttackLw3_ItemManholeExitHitlag','ftGw_AttackLw3_ItemCheckManholeRemove'],
 'ftgamewatchattacks4.c': ['ftGw_AttackS4_ItemTorchSetFlag','ftGw_AttackS4_ItemTorchOnDamage',
    'ftGw_AttackS4_ItemTorchEnterHitlag','ftGw_AttackS4_ItemTorchExitHitlag','ftGw_AttackS4_ItemCheckTorchRemove'],
 'ftgamewatchattackair.c': ['ftGw_AttackAirN_ItemParachuteOnLand','ftGw_AttackAirN_ItemParachuteSetFlag',
    'ftGw_AttackAirN_ItemParachuteRemove','ftGw_AttackAirN_ItemCheckParachuteRemove',
    'ftGw_AttackAirN_ItemTurtleOnLand','ftGw_AttackAirN_ItemTurtleSetFlag','ftGw_AttackAirN_ItemTurtleRemove',
    'ftGw_AttackAirN_ItemCheckTurtleRemove','ftGw_AttackAirN_ItemSparkyOnLand','ftGw_AttackAirN_ItemSparkySetFlag',
    'ftGw_AttackAirN_ItemSparkyRemove','ftGw_AttackAirN_EnterItemHitlag','ftGw_AttackAirN_ExitItemHitlag',
    'ftGw_AttackAirN_ItemCheckSparkyRemove'],
}
EXTRACTED = {
 'src/melee/ft/kinds/ftGameWatch/ftgamewatch.c': ['ftGw_Init_OnDeath','ftGw_Init_OnDamage','ftGw_Init_8014A538',
    'ftGw_Init_OnItemPickup','ftGw_Init_OnItemDrop','ftGw_Init_OnItemVisible','ftGw_Init_OnItemInvisible',
    'ftGw_Init_UnkMotionStates4'],
 'src/melee/it/it_26B1.c':['it_8026B724','it_8026B73C'],
 'src/melee/ft/kinds/ftGameWatch/ftgamewatchspecials.c':[
    'ftGw_SpecialS_ItemJudgementSetFlag','ftGw_SpecialS_ItemJudgementRemove',
    'ftGw_SpecialS_ItemJudgementExitHitlag'],
 'src/melee/ft/kinds/ftGameWatch/ftgamewatchspeciallw.c':[
    'ftGw_SpecialLw_ItemPanicSetFlag','ftGw_SpecialLw_ItemPanicRemove',
    'ftGw_SpecialLw_ItemPanicExitHitlag'],
 'src/melee/ft/kinds/ftGameWatch/ftgamewatchspecialhi.c':[
    'ftGw_SpecialHi_ItemRescueSetNULL','ftGw_SpecialHi_ItemRescueRemove'],
 'src/melee/it/kinds/itgamewatchjudge.c':[
    'itGameWatchJudge_Logic77_Destroyed','it_802C7A84','it_802C7AF0'],
 'src/melee/it/kinds/itgamewatchpanic.c':[
    'itGameWatchPanic_Logic78_Destroyed','it_802C7E94','it_802C7F00'],
 'src/melee/it/kinds/itgamewatchrescue.c':['it_802C8158'],
}


def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()


def extract(source, name):
    match = re.search(r'(?m)^[A-Za-z_][^\n;{}]*\b' + re.escape(name) + r'\([^;]*?\)\s*\{', source)
    if not match: raise RuntimeError('Missing actual original ' + name)
    end = source.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


def run(command, log):
    result = subprocess.run(command, capture_output=True, text=True)
    log.write_text(result.stdout + result.stderr)
    if result.returncode: raise RuntimeError(f'Failed {command[0]} ({result.returncode}): {log}\n{result.stdout}{result.stderr}')
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    snapshot, output = args.snapshot.resolve(), args.output.resolve()
    if any(output == p or p in output.parents for p in [snapshot,HERE]): raise RuntimeError('Output overlaps immutable input')
    for path, expected in json.loads((snapshot / 'freeze.json').read_text()).items():
        if sha(snapshot / path.replace('\\','/')) != expected: raise RuntimeError('Snapshot changed: ' + path)
    output.mkdir(parents=True,exist_ok=True)
    bodies, inputs = [], {}
    paths = ['src/melee/ft/kinds/ftGameWatch/' + name for name in FIGHTERS]
    paths += ['src/melee/it/kinds/' + name for name in ARTICLES]
    for path in paths:
        file = snapshot / 'sdk' / path;inputs[path]=sha(file)
        if file.name in NORMAL_FUNCTIONS:
            bodies.extend(extract(file.read_text(),name) for name in NORMAL_FUNCTIONS[file.name])
        else:
            # Compile exact article policy bodies, excluding the unaccepted
            # allocation/attach constructor and its original global state table.
            source=file.read_text()
            names=re.findall(r'(?m)^(?:static inline |static )?(?:void|bool) ([A-Za-z0-9_]+)\([^;]*?\)\s*\{',source)
            bodies.extend(extract(source,name) for name in names)
    for path,names in EXTRACTED.items():
        file=snapshot/'sdk'/path;inputs[path]=sha(file)
        bodies.extend(extract(file.read_text(),name) for name in names)
    if inputs != json.loads((HERE/'original_inputs.json').read_text()): raise RuntimeError('Original source hashes changed')
    # Headers precede extracted static bodies; exact bodies below are unchanged.
    headers = ['ftgamewatchattack11.h','ftgamewatchattacklw3.h','ftgamewatchattacks4.h',
        'ftgamewatchattackair.h','ftgamewatchspecials.h','ftgamewatchspecialhi.h','ftgamewatchspeciallw.h',
        'itgamewatchgreenhouse.h','itgamewatchmanhole.h','itgamewatchfire.h','itgamewatchparachute.h',
        'itgamewatchturtle.h','itgamewatchbreath.h','itgamewatchjudge.h','itgamewatchpanic.h','itgamewatchrescue.h']
    declarations=''.join(body[:body.index('{')].strip()+';\n' for body in bodies)
    (output/'original.h').write_text(''.join('#include "'+h+'"\n' for h in headers)+declarations+'\n'.join(bodies))
    candidate=compose(output,compare=True)
    generated=output/'behavior.generated.h'
    run([str(snapshot/'bin/native_content_tool.exe'),'script-c',str(candidate),'gamewatch_normals_probe',str(generated)],output/'generate.log')
    text=generated.read_text()
    schema=re.search(r'schema != (0x[0-9a-f]+)ULL',text)[1]
    bits=[]
    for line in (output/'fighter_parameters.fmp').read_text().splitlines():
        kind,name,equals,value=line.split();assert equals=='='
        bits.append(str(struct.unpack('<I',struct.pack('<f',float(value)))[0] if kind=='float' else int(value)&0xFFFFFFFF))
    (output/'parameters.h').write_text('#define FIXTURE_PARAMETER_COUNT '+str(len(bits))+'\n#define FIXTURE_PARAMETER_SCHEMA '+schema+'ULL\nstatic uint32_t fixture_parameter_bits[]={'+','.join(bits)+'};\n')
    # Every unexercised Host operation traps; accepted dependency constructors
    # may compile but cannot silently enter this callback comparison matrix.
    hosts=sorted(set(re.findall(r'host->([a-zA-Z0-9_]+) == 0',text)))
    (output/'unused_hosts.h').write_text(''.join('host.'+h+'=(__typeof__(host.'+h+'))unused;\n' for h in hosts))
    shutil.copyfile(HERE/'compare.c',output/'compare.c')
    vswhere=Path(os.environ.get('ProgramFiles(x86)',r'C:\Program Files (x86)'))/'Microsoft Visual Studio/Installer/vswhere.exe'
    vs=subprocess.run([str(vswhere),'-latest','-products','*','-requires','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],capture_output=True,text=True,check=True).stdout.strip()
    setup=Path(vs)/'VC/Auxiliary/Build/vcvars64.bat'
    command=[str(snapshot/'compiler/clang.exe'),'-std=gnu11','-O1','-ffp-contract=off',
        '-ffunction-sections','-fdata-sections','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES',
        '-D_CRT_SECURE_NO_WARNINGS','-D_DLL','-D_MT','-Xclang','--dependent-lib=msvcrt',
        '-I'+str(snapshot/'sdk/src'),'-I'+str(snapshot/'sdk/extern/dolphin/include'),
        '-I'+str(snapshot/'sdk/src/melee/ft/kinds/ftGameWatch'),
        '-I'+str(snapshot/'sdk/src/melee/it/kinds'),str(output/'compare.c'),
        str(snapshot/'sdk/src/melee/ft/native_callback_lifetime.c'),
        str(HERE.parent/'model_effect_absent_fixture.c'),'-fuse-ld=lld','-Wl,/OPT:REF','-o',str(output/'compare.exe')]
    (output/'compile.cmd').write_text('@echo off\ncall "'+str(setup)+'" -vcvars_ver=14.44 >nul\nif errorlevel 1 exit /b 1\n'+subprocess.list2cmdline(command)+'\n')
    run(['cmd.exe','/d','/c',str(output/'compile.cmd')],output/'compile.log')
    text=run([str(output/'compare.exe')],output/'result.log')
    match=re.search(r'Normals source: (\d+) checks, (\d+) failures, (\d+) pairs; articles(\d+) cleanup(\d+) accessories(\d+) reset(\d+)',text)
    if not match or int(match[2]):raise RuntimeError('Missing or failed source matrix: '+text)
    result=dict(zip(['checks','failures','pairs','articles','cleanup','accessories','reset'],map(int,match.groups())))
    assert result['articles']==39264 and result['cleanup']==4096 and result['accessories']==7168 and result['reset']==3084,result
    receipt={'status':'unregistered controlled normals/props/lifecycle source comparison',
        'snapshot':str(snapshot),'snapshot_sha256':sha(snapshot/'freeze.json'),
        'original_inputs':inputs,'composition_inputs':json.loads((output/'composition_inputs.json').read_text()),
        'fixture_sources':{p.name:sha(p) for p in HERE.iterdir() if p.is_file()},
        'generated_sha256':sha(generated),'results':result,
        'controlled_unexecuted_hosts':hosts,
        'excluded':['Manhole cmd0!=0 child-world wind branch','normal first entry/rapid loop/owned aerial landing setup',
            'constructor allocation and attachment','paper renderer/load palettes/outline/width/asset binding',
            'foreign/reflected prop owner calls','gameplay/pixels/whole fighter activation'],
        'boundaries':['actual callback-source creation-ID registry','item removal invokes original/generated destroyed callback',
            'shared model visibility/state/animation/interaction/group calls controlled',
            'same-author signals controlled by full32bit item ID, invoking actual generated signals']}
    (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(text.strip());print('Receipt:',output/'receipt.json')


if __name__=='__main__':main()
