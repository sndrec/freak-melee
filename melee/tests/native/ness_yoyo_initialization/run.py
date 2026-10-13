"""Generate/compile actual YoYo initialization comparison against one frozen SDK."""
from pathlib import Path
import argparse, hashlib, json, os, re, shutil, subprocess

HERE=Path(__file__).resolve().parent
ORIGINALS={
    'src/melee/mp/mpcoll.c':['mpColl_80041EE4','mpColl_SetECBSource_Fixed'],
    'src/melee/it/kinds/inlines.h':['Item_InitSpawn'],
    'src/melee/it/it_2725.c':['it_80275328'],
    'src/melee/it/kinds/itlinkhookshot.c':['it_802A2474','it_802A24A0','it_802A24D0'],
    'src/melee/it/kinds/itnessyoyo.c':['it_802BE65C_LoadString','it_802BE65C','it_802BE9D8','itNessYoyo_Logic59_PickedUp'],
}
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def dump(path,value):path.write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8')
def extract(text,name):
    match=re.search(r'(?m)^[^\n;{}]*\b'+re.escape(name)+r'\([^;]*?\)\s*\{',text)
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
    originals='';inputs={}
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
    run([str(tool),'script-c',str(output/'behavior.fms'),'ness_yoyo_initialization',str(output/'behavior.generated.h')],output/'generate.log')
    vswhere=Path(os.environ.get('ProgramFiles(x86)',r'C:\Program Files (x86)'))/'Microsoft Visual Studio/Installer/vswhere.exe'
    result=subprocess.run([str(vswhere),'-latest','-products','*','-requires','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],capture_output=True,text=True,check=True)
    setup=Path(result.stdout.strip())/'VC/Auxiliary/Build/vcvars64.bat'
    command=[str(clang),'-std=gnu11','-O1','-ffp-contract=off','-ffunction-sections','-fdata-sections',
             '-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-D_DLL','-D_MT',
             '-Xclang','--dependent-lib=msvcrt','-I'+str(snapshot/'sdk/src'),'-I'+str(snapshot/'sdk/extern/dolphin/include'),
             str(output/'compare.c'),str(snapshot/'sdk/src/melee/lb/native_model_joint.c'),
             '-fuse-ld=lld','-Wl,/OPT:REF','-o',str(output/'compare.exe')]
    (output/'compile.cmd').write_text('@echo off\ncall "'+str(setup)+'" -vcvars_ver=14.44 >nul\nif errorlevel 1 exit /b 1\n'+subprocess.list2cmdline(command)+'\n',encoding='utf-8')
    run(['cmd.exe','/d','/c',str(output/'compile.cmd')],output/'compile.log')
    text=run([str(output/'compare.exe')],output/'result.log')
    match=re.search(r'(\d+) checks, (\d+) failures, (\d+) source pairs, (\d+) defined failure cases',text)
    if not match or tuple(map(int,match.groups()[1:]))!=(0,504,60):raise RuntimeError('Unexpected result scope: '+text)
    dump(output/'receipt.json',{'status':'unregistered initialization source comparison only',
        'snapshot':str(snapshot),'snapshot_sha256':sha(snapshot/'freeze.json'),
        'originals':inputs,'production_resource_sha256':sha(snapshot/'sdk/src/melee/it/native_article_resource.c'),
        'production_joint_sha256':sha(snapshot/'sdk/src/melee/lb/native_model_joint.c'),
        'fixture_sources':{p.name:sha(p) for p in HERE.iterdir() if p.is_file()},
        'generated_sha256':sha(output/'behavior.generated.h'),'results':{'checks':int(match[1]),'failures':0,'source_pairs':504,'defined_failure_cases':60},
        'boundaries':['HSD allocation/model graph/animation/render','active Definition/creator generation lookup','item engine allocation/attachment/state'],
        'pending':['YoYo four physics solvers','smash phases and common presentation','actual DAT model import/rendering gameplay']})
    print(text.strip());print('Receipt:',output/'receipt.json')
if __name__=='__main__':main()
