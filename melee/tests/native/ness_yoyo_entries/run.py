"""Compare unregistered YoYo entries/material/animation and both Bat states against frozen source."""
from pathlib import Path
import argparse, hashlib, json, os, re, shutil, subprocess, struct

HERE=Path(__file__).resolve().parent
ORIGINALS={
    'src/melee/ft/ftlib.c':['ftLib_8008741C','ftLib_80087460'],
    'src/melee/mp/mpcoll.c':['mpColl_80041EE4','mpColl_SetECBSource_Fixed'],
    'src/melee/it/it_2725.c':['it_80275328'],
    'src/melee/it/kinds/itlinkhookshot.c':['it_802A2474','it_802A24A0','it_802A24D0','it_802A3C98'],
    'src/melee/ft/kinds/ftNess/ftnessattackhi4.c':['ftNs_AttackHi4_YoyoItemSetFlag','ftNs_AttackHi4_YoyoItemSetUnk2'],
    'src/melee/ft/kinds/ftNess/ftnessattacks4.c':['ftNs_AttackS4_CheckNessBatRemove','ftNs_AttackS4_ItemNessBatSetNULL'],
    'src/melee/ft/ft_0BF0.c':['ftCo_800BF228'],
    'src/melee/it/kinds/itnessyoyo.c':['it_802BE5B8','it_802BE5D8','it_802BE65C_LoadString','it_802BE65C','it_802BE958_inline','itNessyoyo_UnkMotion3_Anim_inline','itNessyoyo_UnkMotion3_Anim_UpdateRotation','itNessyoyo_UnkMotion3_Anim','it_802BFE5C','my_sqrtf','it_802BFEC4','it_802C0010','it_2725_Logic59_EvtUnk'],
    'src/melee/it/kinds/itnessbat.c':['itNessbat_ClearOwnerRef','itNessbat_RemoveItem','itNessbat_UnkMotion0_Anim','itNessbat_UnkMotion0_Phys','itNessbat_UnkMotion0_Coll','copy_jobj_scale','itNessbat_UnkMotion1_Anim','itNessbat_UnkMotion1_Phys','itNessbat_UnkMotion1_Coll','itNessBat_Logic58_EvtUnk'],
}
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def dump(path,value):path.write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8')
def extract(text,name):
    # A definition begins at column zero; an indented if/call is not a body.
    match=re.search(r'(?m)^[A-Za-z_][^\n;{}]*\b'+re.escape(name)+r'\([^;]*?\)\s*(?://[^\n]*\n\s*)?\{',text)
    if not match:raise RuntimeError('Original function not found: '+name)
    opening=text.index('{',match.start());end=opening+1;depth=1
    while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[match.start():end]+'\n\n'
def run(command,log):
    result=subprocess.run(command,capture_output=True,text=True)
    log.write_text(result.stdout+result.stderr,encoding='utf-8')
    if result.returncode:raise RuntimeError(f'Failed({result.returncode}): {log}\n{result.stdout+result.stderr}')
    return result.stdout
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args();snapshot=args.snapshot.resolve();output=args.output.resolve()
    if any(output==p or p in output.parents for p in [snapshot,HERE]):raise RuntimeError('Output cannot modify snapshot/tracked source.')
    frozen={k.replace('\\','/'):v for k,v in json.loads((snapshot/'freeze.json').read_text()).items()}
    for key,value in frozen.items():
        if key.startswith(('sdk/','compiler/')) or key=='bin/native_content_tool.exe':
            if sha(snapshot/key)!=value:raise RuntimeError('Snapshot hash mismatch: '+key)
    output.mkdir(parents=True,exist_ok=True)
    originals=(HERE/'reference_math.h').read_text();inputs={}
    math_receipt=json.loads((HERE/'math_original_inputs.json').read_text())
    if sha(HERE/'reference_math.h')!=math_receipt['fragment_sha256']:raise RuntimeError('Math reference hash mismatch.')
    expected=json.loads((HERE/'original_inputs.json').read_text())
    for path,names in ORIGINALS.items():
        source=snapshot/'sdk'/path;inputs[path]=sha(source);text=source.read_text()
        if inputs[path]!=expected[path]:raise RuntimeError('Original reference changed: '+path)
        for name in names:originals+=extract(text,name)
    (output/'original.h').write_text(originals,encoding='utf-8');dump(output/'original_inputs.json',inputs)
    for name in ['compare.c','fighter_parameters.fmp']:
        shutil.copyfile(HERE/name,output/name)
    source=(HERE/'candidate.fms').read_text()
    # Attribute metadata stays in the authoring draft; gameplay uses owned params.
    source='\n'.join(line for line in source.splitlines() if not line.startswith('parameter '))+'\n'
    (output/'behavior.fms').write_text(source,encoding='utf-8')
    tool=snapshot/'bin/native_content_tool.exe';clang=snapshot/'compiler/clang.exe'
    run([str(tool),'script-c',str(output/'behavior.fms'),'ness_yoyo_entries',str(output/'behavior.generated.h')],output/'generate.log')
    schema=re.search(r'schema != (0x[0-9a-f]+)ULL',(output/'behavior.generated.h').read_text())[1]
    fields=[]
    for line in (HERE/'fighter_parameters.fmp').read_text().splitlines():
        if not line.strip():continue
        kind,name,equals,value=line.split()
        bits=struct.unpack('<I',struct.pack('<f',float(value)))[0] if kind=='float' else int(value)&0xFFFFFFFF
        fields.append('    {"'+name+'", UINT32_C('+str(bits)+')},')
    (output/'parameters.h').write_text('#define FIXTURE_PARAMETER_SCHEMA '+schema+'ULL\nstatic const struct {const char* name; uint32_t bits;} fixture_parameters[] = {\n'+'\n'.join(fields)+'\n};\n')
    vswhere=Path(os.environ.get('ProgramFiles(x86)',r'C:\Program Files (x86)'))/'Microsoft Visual Studio/Installer/vswhere.exe'
    result=subprocess.run([str(vswhere),'-latest','-products','*','-requires','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],capture_output=True,text=True,check=True)
    setup=Path(result.stdout.strip())/'VC/Auxiliary/Build/vcvars64.bat'
    command=[str(clang),'-std=gnu11','-O1','-ffp-contract=off','-ffunction-sections','-fdata-sections',
             '-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-D_DLL','-D_MT',
             '-Xclang','--dependent-lib=msvcrt','-I'+str(snapshot/'sdk/src'),'-I'+str(snapshot/'sdk/extern/dolphin/include'),
             str(output/'compare.c'),str(snapshot/'sdk/src/melee/lb/native_model_joint.c'),
             str(snapshot/'sdk/src/melee/ft/native_fighter_target.c'),
             '-fuse-ld=lld','-Wl,/OPT:REF','-o',str(output/'compare.exe')]
    (output/'compile.cmd').write_text('@echo off\ncall "'+str(setup)+'" -vcvars_ver=14.44 >nul\nif errorlevel 1 exit /b 1\n'+subprocess.list2cmdline(command)+'\n',encoding='utf-8')
    run(['cmd.exe','/d','/c',str(output/'compile.cmd')],output/'compile.log')
    text=run([str(output/'compare.exe')],output/'result.log')
    match=re.search(r'(\d+) checks, (\d+) failures, (\d+) source pairs; max difference ([0-9.e+-]+); sqrt difference ([0-9.e+-]+)',text)
    if not match or int(match[2])!=0 or int(match[3])!=2604:raise RuntimeError('Unexpected result scope: '+text)
    coverage=re.search(r'Coverage: entries(\d+) material(\d+) yoyo_animation(\d+) bat(\d+) interaction(\d+)',text)
    if not coverage or tuple(map(int,coverage.groups()))!=(1078,12,1152,360,2):raise RuntimeError('Coverage gate failed: '+text)
    safety=re.search(r'Owner safety: (\d+) authored-only cases',text)
    if not safety or int(safety[1])!=16:raise RuntimeError('Owner safety gate failed: '+text)
    dump(output/'receipt.json',{'status':'unregistered article entries/animation source comparison only',
        'snapshot':str(snapshot),'snapshot_sha256':sha(snapshot/'freeze.json'),
        'originals':inputs,'production_resource_sha256':sha(snapshot/'sdk/src/melee/it/native_article_resource.c'),
        'production_joint_sha256':sha(snapshot/'sdk/src/melee/lb/native_model_joint.c'),
        'production_target_sha256':sha(snapshot/'sdk/src/melee/ft/native_fighter_target.c'),
        'fixture_sources':{p.name:sha(p) for p in HERE.iterdir() if p.is_file()},
        'math_originals':math_receipt,'generated_sha256':sha(output/'behavior.generated.h'),
        'results':{'checks':int(match[1]),'failures':0,'source_pairs':int(match[3]),'coverage':dict(zip(['entries','material','yoyo_animation','bat','interaction'],map(int,coverage.groups()))),'maximum_absolute_difference':float(match[4]),'maximum_sqrt_difference':float(match[5])},
        'owner_safety_cases':int(safety[1]),
        'boundaries':['HSD allocation/render/animation evaluator','active Definition and creator lookup','item state transition/interaction/signals','subtree hidden flags engine helpers'],
        'pending':['smash entry/phases and complete default lifecycle activation','actual DAT models/rendered gameplay','historic identity cannot be recovered from a recycled raw owner address']})
    print(text.strip());print('Receipt:',output/'receipt.json')
if __name__=='__main__':main()
