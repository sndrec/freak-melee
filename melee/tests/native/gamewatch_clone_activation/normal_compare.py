"""Full generated GameWatch normal attacks vs actual original bodies, console only.

Reuses an existing real clone as read-only retail input; no repeated package
ownership or gameplay test. All compiler, engine types and source bodies come
from the requested immutable snapshot. No fake replacement Definition.
"""
import argparse,ast,hashlib,json,re,shutil,struct,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
from assemble import sha
from prepare import declaration
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]

def extract(text,name):
    m=re.search(r'(?m)^[A-Za-z_][^\n;{}]*\b'+re.escape(name)+r'\([^;]*?\)\s*\{',text)
    if not m: raise ValueError('missing actual definition '+name)
    end=text.index('{',m.start())+1;depth=1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[m.start():end]+'\n'

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--snapshot',required=True,type=Path)
    ap.add_argument('--project',required=True,type=Path)
    ap.add_argument('--candidate',required=True,type=Path)
    ap.add_argument('--output',required=True,type=Path)
    ap.add_argument('--bucket-only',action='store_true',help='Run only original/generated bucket visibility across charges and saved/current selections')
    a=ap.parse_args();snap=a.snapshot.resolve();out=a.output.resolve()
    if out.exists() or any(p==out or p in out.parents for p in (snap,HERE,a.project.resolve())):ap.error('fresh independent output required')
    manifest=json.loads((snap/'freeze.json').read_text())
    def verify():
        for name,digest in manifest.items():
            if sha(snap/name)!=digest:raise ValueError('snapshot changed '+name)
    verify();out.mkdir(parents=True);commands=[]
    def run(stage,cmd):
        cmd=list(map(str,cmd));r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=180)
        (out/(stage+'.log')).write_text(r.stdout+r.stderr,encoding='utf-8',newline='\n')
        commands.append({'stage':stage,'command':cmd,'exit':r.returncode})
        (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n',newline='\n')
        if r.returncode:raise RuntimeError(stage+' failed:\n'+(r.stdout+r.stderr)[-12000:])
        return r.stdout
    project=out/'project';shutil.copytree(a.project,project)
    # Import a fresh candidate into this private copy, preserving source clone.
    for name in ('behavior.fms','fighter_parameters.fmp'):(project/name).unlink()
    tool=snap/'bin/native_content_tool.exe'
    run('import',[tool,'import-parameters',project,a.candidate,project/'behavior.fms'])
    generated=out/'behavior.generated.h'
    run('generate',[tool,'script-c',project/'behavior.fms','gamewatch_normal_phase_probe',generated])
    code=generated.read_text();schema=re.search(r'schema != (0x[0-9a-f]+)ULL',code)[1]
    bits=[]
    for row in (project/'fighter_parameters.fmp').read_text().splitlines():
        kind,name,equal,value=row.split();assert equal=='='
        bits.append(str(struct.unpack('<I',struct.pack('<f',float(value)))[0] if kind=='float' else int(value)&0xffffffff))
    (out/'parameters.h').write_text('#define FIXTURE_PARAMETER_COUNT '+str(len(bits))+'\n#define FIXTURE_PARAMETER_SCHEMA '+schema+'ULL\nstatic uint32_t fixture_parameter_bits[]={'+','.join(bits)+'};\n',newline='\n')
    hosts=sorted(set(re.findall(r'host->(\w+)\s*==\s*0',code)))
    (out/'unused_hosts.h').write_text(''.join('host.'+h+'=(__typeof__(host.'+h+'))unused;\n' for h in hosts),newline='\n')
    # Reuse the accepted source-body inventory, then extend with all twelve
    # actual normal state phases and entries. Do not import its stale runner.
    accepted=HERE.parent/'gamewatch_normals_composition/run.py'
    values={}
    for node in ast.parse(accepted.read_text()).body:
        if isinstance(node,ast.Assign) and isinstance(node.targets[0],ast.Name) and node.targets[0].id in ('EXTRACTED','NORMAL_FUNCTIONS','ARTICLES','FIGHTERS'):
            values[node.targets[0].id]=ast.literal_eval(node.value)
    inventory=values['EXTRACTED'];bodies=[];inputs={}
    for name in values['FIGHTERS']:
        path='src/melee/ft/kinds/ftGameWatch/'+name;file=snap/'sdk'/path;source=file.read_text();inputs[path]=sha(file)
        names=values['NORMAL_FUNCTIONS'][name]
        for match in re.finditer(r'(?m)^(?:static )?(?:void|bool) (ftGw_\w+)\([^;]*?\)\s*\{',source):
            n=match[1]
            if re.search(r'_(?:Enter|Anim|IASA|Phys|Coll|Init)$',n) and n not in names:names.append(n)
        if name=='ftgamewatchattackair.c':names.append('ftGw_AttackAirN_DecideAction')
        bodies.extend(extract(source,n) for n in names)
    for name in values['ARTICLES']:
        path='src/melee/it/kinds/'+name;file=snap/'sdk'/path;source=file.read_text();inputs[path]=sha(file)
        names=re.findall(r'(?m)^(?:static inline |static )?(?:void|bool) (\w+)\([^;]*?\)\s*\{',source)
        bodies.extend(extract(source,n) for n in names)
    inventory.update({'src/melee/ft/kinds/ftCommon/ftCo_AttackAir.c':['ftCo_AttackAir_Anim'],
        'src/melee/ft/kinds/ftCommon/ftCo_Landing.c':['ftCo_Landing_Anim','ftCo_Landing_Phys','ftCo_Landing_Coll'],
        'src/melee/ft/kinds/ftCommon/ftCo_Attack100.c':['ftCo_Attack100End_Anim']})
    if a.bucket_only:
        inventory['src/melee/ft/kinds/ftGameWatch/ftgamewatchspeciallw.c'].append('ftGw_SpecialLw_UpdateBucketModel')
        file=snap/'sdk/src/melee/ft/ftparts.c'
        inputs['src/melee/ft/ftparts.c']=sha(file)
        (out/'bucket_original.h').write_text(extract(file.read_text(),'ftParts_80074A4C')+extract(file.read_text(),'ftParts_80074B0C'),newline='\n')
    for path,names in inventory.items():
        file=snap/'sdk'/path;inputs[path]=sha(file);bodies.extend(extract(file.read_text(),n) for n in names)
    headers=['ftgamewatchattack11.h','ftgamewatchattack100.h','ftgamewatchattacklw3.h','ftgamewatchattacks4.h','ftgamewatchattackair.h','ftgamewatchspecials.h','ftgamewatchspecialhi.h','ftgamewatchspeciallw.h','itgamewatchgreenhouse.h','itgamewatchmanhole.h','itgamewatchfire.h','itgamewatchparachute.h','itgamewatchturtle.h','itgamewatchbreath.h','itgamewatchjudge.h','itgamewatchpanic.h','itgamewatchrescue.h']
    (out/'original.h').write_text(''.join('#include "'+h+'"\n' for h in headers)+''.join(b[:b.index('{')].strip()+';\n' for b in bodies)+'\n'.join(bodies),newline='\n')
    basepath=snap/'sdk/src/melee/ft/kinds/ftGameWatch/ftgamewatch_clone.c'
    stockpath=snap/'sdk/src/melee/ft/kinds/ftGameWatch/ftgamewatch.c'
    table=declaration(stockpath.read_text(),'MotionState ftGw_Init_MotionStateTable')
    base=basepath.read_text();decls='\n'.join(declaration(base,p) for p in ('static const NativeFighterBone ftGameWatchClone_Bones','static const NativeFighterArticleAsset ftGameWatchClone_Articles','static const Fighter_DemoStrings ftGameWatchClone_DemoMotionNames','const NativeFighterDefinition ftGameWatchClone_Definition'))
    pointers=sorted(set(re.findall(r'\bftGw_\w+\b',table+decls)))
    pointers=[n for n in pointers if not any(part in n for part in ('_MS_','_MF_','_SM_','_MotionStateTable'))]+['ftCamera_UpdateCameraBox']
    (out/'base.h').write_text(''.join('#define '+n+' stock_slot\n' for n in pointers)+table+'\n'+decls+'\n'+''.join('#undef '+n+'\n' for n in pointers),newline='\n')
    for name in ('normal_compare.c','normal_boundaries.h','normal_matrix.h'):shutil.copyfile(HERE/name,out/name)
    cmd=[snap/'compiler/clang.exe','-std=gnu11','-O1','-ffp-contract=off','-ffunction-sections','-fdata-sections','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(snap/'sdk/src'),'-I'+str(snap/'sdk/extern/dolphin/include'),'-I'+str(snap/'sdk/src/melee/ft/kinds/ftGameWatch'),'-I'+str(snap/'sdk/src/melee/it/kinds'),out/'normal_compare.c',snap/'sdk/src/melee/ft/native_callback_lifetime.c',HERE.parent/'model_effect_absent_fixture.c','-fuse-ld=lld','-Wl,/OPT:REF','-o',out/'compare.exe']
    if a.bucket_only:cmd.insert(1,'-DBUCKET_ONLY=1')
    run('compile',cmd);result=run('matrix',[out/'compare.exe']);verify()
    if a.bucket_only:
        summary=re.search(r'Bucket current visibility source: (\d+) checks, (\d+) failures, (\d+) pairs',result)
        if not summary or int(summary[2]):raise RuntimeError('bad bucket summary')
        receipt={'status':'PASS actual original/generated BucketModels with actual saved/current group operations','snapshot':str(snap),'freeze_sha256':sha(snap/'freeze.json'),'checks':int(summary[1]),'failures':int(summary[2]),'pairs':int(summary[3]),'commands':commands,'original_sources':inputs,'namespace_sources':{p.name:sha(p) for p in HERE.iterdir() if p.is_file()},'generated_sha256':sha(generated),'source_sha256':sha(a.candidate),'boundaries':['actual original UpdateBucketModel and actual ftParts_80074A4C/80074B0C bodies','actual generated complete module/imported owned parameters','controlled DObj refresh callback counts; no actual draw/pixels','charges -1,0,1,2,3,4,INT_MAX and 32 saved/current initial selections'],'exclusions':['unchanged full normal matrix is not rerun','no gameplay or GX/pixels']}
        (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',newline='\n');print(result);print(out/'receipt.json');return
    total=re.search(r'Full normals source: (\d+) checks, (\d+) failures, (\d+) pairs',result)
    breakdown=re.search(r'Normal new phases: (\d+) motion pairs, (\d+) entry pairs, (\d+) landing pairs',result)
    if not total or not breakdown or int(total[2]):raise RuntimeError('bad matrix summary')
    receipt={'status':'PASS controlled actual original/full generated normal phase comparison','snapshot':str(snap),'freeze_sha256':sha(snap/'freeze.json'),'checks':int(total[1]),'failures':int(total[2]),'pairs':int(total[3]),'normal_motion_pairs':int(breakdown[1]),'entry_pairs':int(breakdown[2]),'landing_pairs':int(breakdown[3]),'commands':commands,'original_sources':inputs,'namespace_sources':{p.name:sha(p) for p in HERE.iterdir() if p.is_file()},'generated_sha256':sha(generated),'source_sha256':sha(a.candidate),'boundaries':['real copied-base metadata/table, nonexecuted stock entry sentinels','real imported retail owned parameters; actual generated C callbacks','actual native callback generation registry','controlled shared transition/physics/input/collision/pose boundaries','setup accessory binding identity checked; accessory constructors not executed here','controlled owned signals invoke actual generated article callbacks; removal invokes actual original/generated destruction callback'],'exclusions':['engine package loader rejected SDK64 NULL-matanim; no claimed package acceptance','GameWatch-specific muted-channel exception closure','allocation/attachment/full common aerial entry internals','GX/pixels/gameplay']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',newline='\n');print(result);print(out/'receipt.json')
if __name__=='__main__':main()
