"""Compare a compiled Mewtwo candidate with mechanically extracted original C.

This compiles only the direct comparison harness; it never starts the game or
rebuilds shared targets. All production source and headers come from one frozen
SDK. The original jump source must predate the shared jump-core refactor.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess
import struct
import sys
from mewtwo_special_parity import root_offset

ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sdk-root',type=Path,required=True,help='Frozen matching SDK C/header tree')
parser.add_argument('--project',type=Path,required=True,help='Imported project with generated behavior C and module')
parser.add_argument('--script',type=Path,required=True,help='Imported owned behavior source used to parse state layout')
parser.add_argument('--original-jump',type=Path,required=True,help='Original pre-refactor ftCo_JumpAerial.c, mechanically saved from source history')
parser.add_argument('--original-screw',type=Path,required=True)
parser.add_argument('--original-multijump',type=Path,required=True)
parser.add_argument('--common-data',type=Path,required=True,help='Raw ISO PlCo.dat for stock semantic joint maps')
parser.add_argument('--out-dir',type=Path,required=True)
args=parser.parse_args()
SDK=args.sdk_root.resolve();PROJECT=args.project.resolve();SCRIPT=args.script.resolve()
OUT=args.out_dir.resolve();OUT.mkdir(parents=True,exist_ok=True)

def braces(source,start):
    opening=source.index('{',start)
    depth=1
    cursor=opening+1
    while depth:
        depth += (source[cursor]=='{')-(source[cursor]=='}')
        cursor+=1
    return opening,cursor

def extract(source,name):
    match=re.search(r'(?m)^[^\n;{}]*\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',source)
    assert match,name
    _,end=braces(source,match.start())
    return source[match.start():end]

cpu_path=SDK/'src/melee/ft/kinds/ftCommon/ftCo_0A01.c'
cpu=cpu_path.read_text()
helpers=[]
for name in ('ftCo_CpuSetNeutralStick','ftCo_CpuTapB','ftCo_CpuStartCharging','ftCo_CpuTapR','ftCo_CpuTapRAndWait'):
    start=cpu.index('static inline void '+name+'(')
    _,end=braces(cpu,start)
    helpers.append(cpu[start:end])
reference=['#include <melee/ft/ftcmdscript.h>','#include <melee/ft/kinds/ftMewtwo/forward.h>',*helpers]
for function_name,name in (('ftCo_800ACD5C','decision'),('ftCo_800ADC28','cancel')):
    source_start=re.search(r'\nvoid '+function_name+r'\(Fighter\* fp\)\s*\{',cpu).start()
    start=cpu.index('else if (fp->kind == Ft_Kind_Mewtwo)',source_start)
    opening,end=braces(cpu,start)
    reference.append('static void stock_cpu_charge_'+name+'(Fighter* fp) '+cpu[opening:end])
dynamics_path=SDK/'src/melee/ft/ftdynamics.c'
dynamics=dynamics_path.read_text()
start=dynamics.index('        FighterKind kind;')
end=dynamics.index('#ifdef MELEE_NATIVE',start)
reference.append('static bool stock_dynamics_ground(Fighter* fp) { bool ret_B0;\n'+dynamics[start:end]+'return ret_B0; }')
(OUT/'mewtwo_cpu_reference.generated.h').write_text('\n\n'.join(reference)+'\n')
common_path=args.common_data.resolve(); common=common_path.read_bytes()
common_root=root_offset(common,'ftLoadCommonData')
ptr=lambda slot:32+struct.unpack_from('>I',common,slot)[0]
part_descriptor=ptr(ptr(common_root+16)+16*4)
joint_count=struct.unpack_from('>I',common,part_descriptor+8)[0]
joints=common[ptr(part_descriptor):ptr(part_descriptor)+joint_count]
parts=common[ptr(part_descriptor+4):ptr(part_descriptor+4)+54]
(OUT/'mewtwo_stock_parts.generated.h').write_text(
    'static u8 stock_joint_map[]={'+','.join(map(str,joints))+'};\n'+
    'static u8 stock_part_map[]={'+','.join(map(str,parts))+'};\n')
generated=(PROJECT/'behavior.generated.c').read_text()
required=sorted(set(re.findall(r'host->(\w+) == 0',generated)))
(OUT/'mewtwo_required_host.generated.h').write_text(''.join('host.'+name+'=(__typeof__(host.'+name+'))unused_host;\n' for name in required))
compiler=Path('C:/Users/loser/scoop/apps/llvm/current/bin/clang.exe')
flags=['-std=gnu11','-O2','-ffp-contract=off','-ffunction-sections','-fdata-sections','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-D_DLL','-D_MT','-Xclang','--dependent-lib=msvcrt','-I'+str(SDK/'src'),'-I'+str(SDK/'extern/dolphin/include'),'-I'+str(SDK/'tools'),'-I'+str(OUT)]
reference_functions={
    'src/melee/ft/kinds/ftMewtwo/ftmewtwospecialn.c':('ftMewtwo_SpecialN_LaunchShadowBall','ftMt_SpecialN_Shoot'),
    'src/melee/it/kinds/itmewtwoshadowball.c':('it_802C5000','it_802C519C'),
    'src/melee/ft/kinds/ftMewtwo/ftmewtwo.c':('ftMt_Init_OnItemPickup','ftMt_Init_OnItemDrop','ftMt_Init_OnItemInvisible','ftMt_Init_OnItemVisible','ftMt_Init_OnKnockbackEnter','ftMt_Init_OnKnockbackExit','ftMt_Init_OnLoad'),
    'src/melee/ft/ft_0892.c':('ft_80089914',),
    'src/melee/ft/ftparts_table.c':('ftParts_GetBoneIndex',),
    'src/melee/ft/ftcommon.c':('ftCommon_8007D5D4',),
    'src/melee/ft/kinds/ftCommon/ftCo_Attack100.c':('ftCo_800D7268',),
    'tools/native_fighter_script_state.c':('identifier','nativeFighterScript_StateField','nativeFighterScript_AddStateField'),
}
sources=[ROOT/'tests/native/mewtwo_stock_policy_test.c',SDK/'src/melee/ft/native_fighter_parameters.c']
originals=[]
for index,(relative,names) in enumerate(reference_functions.items()):
    path=SDK/relative
    originals.append(path)
    stock=path.read_text()
    includes='\n'.join(line for line in stock.splitlines() if line.startswith('#include '))
    reference_path=OUT/('stock_'+str(index)+'.generated.c')
    selected='\n\n'.join(extract(stock,name) for name in names)
    if relative.endswith('/ftcommon.c'): selected=selected.replace('ftCommon_8007D5D4','stock_ground_to_air')
    reference_path.write_text(includes+'\n\n'+selected+'\n')
    sources.append(reference_path)
    flags.append('-I'+str(path.parent))
old_path=args.original_jump.resolve()
current_path=SDK/'src/melee/ft/kinds/ftCommon/ftCo_JumpAerial.c'
old=old_path.read_text(); current=current_path.read_text()
includes='\n'.join(line for line in current.splitlines() if line.startswith('#include '))
old_functions='\n\n'.join(extract(old,name) for name in ('ft_SetVec','ft_JumpAerial_Sound','ftCo_800CBAC4','ftMt_JumpAerial_Enter'))
old_functions=old_functions.replace('ftCo_800CBAC4','stock_original_aerial_begin').replace('ftMt_JumpAerial_Enter','stock_original_mt_jump')
old_reference=OUT/'stock_original_jump.generated.c'
old_reference.write_text(includes+'\n\n'+old_functions+'\n')
current_reference=OUT/'current_jump.generated.c'
current_reference.write_text(includes+'\n\n'+extract(current,'ft_SetVec')+'\n\n'+extract(current,'ftCo_JumpAerial_Begin')+'\n\n'+extract(current,'ft_800CB6EC')+'\n')
sources.extend((old_reference,current_reference)); originals.extend((old_path,current_path))
flags.append('-I'+str(current_path.parent))
jump_prototype='void stock_original_aerial_begin(Fighter_GObj*,FtMotionId,Vec3*,bool);\n'
for original,relative,names,renames in (
    (args.original_screw,'src/melee/ft/kinds/ftCommon/ftCo_ItemScrew.c',('ft_800D2E7C',),{'ft_800D2E7C':'stock_original_screw'}),
    (args.original_multijump,'src/melee/ft/kinds/ftCommon/ftCo_JumpAerialF1.c',('ftCo_800D74A4_inline','ftCo_800D74A4'),{'ftCo_800D74A4':'stock_original_multijump'})):
    for before in (True,False):
        path=original.resolve() if before else SDK/relative
        text=path.read_text();includes='\n'.join(line for line in text.splitlines() if line.startswith('#include '))
        selected='\n\n'.join(extract(text,name) for name in names)
        if before:
            selected=selected.replace('ftCo_800CBAC4','stock_original_aerial_begin')
            for old_name,new_name in renames.items():selected=selected.replace(old_name,new_name)
        generated_path=OUT/(('original_' if before else 'current_')+Path(relative).stem+'.generated.c')
        generated_path.write_text(includes+'\n\n'+jump_prototype+selected+'\n');sources.append(generated_path);originals.append(path);flags.append('-I'+str((SDK/relative).parent))
objects=[]
for index,path in enumerate(sources):
    obj=OUT/(str(index)+'.obj')
    result=subprocess.run([str(compiler),*flags,'-c',str(path),'-o',str(obj)],capture_output=True,text=True)
    (OUT/(str(index)+'.compile.log')).write_text(result.stdout+result.stderr)
    if result.returncode:
        print(result.stdout+result.stderr)
        raise SystemExit(result.returncode)
    objects.append(str(obj))
exe=OUT/'mewtwo_throw_cpu_stock_test.exe'
result=subprocess.run([str(compiler),'-fuse-ld=lld',*objects,'-o',str(exe),'-Wl,/OPT:REF'],capture_output=True,text=True)
(OUT/'link.log').write_text(result.stdout+result.stderr)
if result.returncode:
    print(result.stdout+result.stderr)
    raise SystemExit(result.returncode)
result=subprocess.run([str(exe),str(PROJECT/'module.dll'),str(PROJECT/'fighter_parameters.fmp'),str(SCRIPT)],capture_output=True,text=True)
(OUT/'comparison.log').write_text(result.stdout+result.stderr)
(OUT/'source_manifest.json').write_text(json.dumps({str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in [common_path,cpu_path,dynamics_path,*originals,*sources,SCRIPT,PROJECT/'module.dll',PROJECT/'fighter_parameters.fmp',PROJECT/'behavior.generated.c']},indent=2))
print((result.stdout+result.stderr)[:15000])
raise SystemExit(result.returncode)
