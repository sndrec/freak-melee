"""Inventory the actual normal-clone graphs/rig and frozen stock callback tables."""
import argparse, hashlib, json, re, subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def initializer(text,name):
    m=re.search(r'^.*\b'+re.escape(name)+r'\s*\[[^;]*?=\s*\{',text,re.M)
    if not m:raise RuntimeError('missing initializer '+name)
    start=text.index('{',m.start());i=start+1;depth=1
    while depth:depth+=(text[i]=='{')-(text[i]=='}');i+=1
    source=re.sub(r'/\*[\s\S]*?\*/|//[^\n]*','',text[start+1:i-1])
    entries=[];depth=0;start=0
    for j,ch in enumerate(source):
        depth+=(ch in '({[')-(ch in ')}]')
        if ch==',' and not depth:entries.append(source[start:j].strip());start=j+1
    tail=source[start:].strip()
    if tail:entries.append(tail)
    return entries,text.count('\n',0,m.start())+1

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--snapshot',required=True,type=Path)
    p.add_argument('--projects',required=True,type=Path,help='normal preparation output containing link and young_link')
    p.add_argument('--common',type=Path,default=ROOT/'.local/mewtwo_common_stock.dat')
    p.add_argument('--output',required=True,type=Path)
    a=p.parse_args();snap=a.snapshot.resolve();out=a.output.resolve();projects=a.projects.resolve()
    if out.exists() or snap in out.parents:p.error('fresh output outside snapshot required')
    out.mkdir(parents=True);sdk=snap/'sdk';manifest=json.loads((snap/'freeze.json').read_text())
    frozen={n:h for n,h in manifest.items() if n.replace('\\','/').startswith(('sdk/','compiler/'))}
    def verify():
        for n,h in frozen.items():
            if sha(snap/n)!=h:raise RuntimeError('changed frozen input '+n)
    verify();commands=[]
    def run(stage,cmd):
        cmd=list(map(str,cmd));r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120)
        (out/(stage+'.log')).write_bytes((r.stdout+r.stderr).encode());commands.append(dict(stage=stage,command=cmd,exit=r.returncode))
        (out/'commands.json').write_bytes((json.dumps(commands,indent=2)+'\n').encode())
        if r.returncode:raise RuntimeError(stage+' failed:\n'+r.stderr)
        return r.stdout
    files=[sdk/'src'/s for s in ['melee/ft/ftcharacter_assets.c','melee/ft/ftdat.c','melee/ft/ftaction_data.c','melee/it/itarticle_data.c','melee/it/itcommand_data.c',*[f'sysdolphin/baselib/dat{s}.c' for s in ('','_import','_model','_material','_geometry','_animation','_scene','_texture')]]]
    flags=[snap/'compiler/clang.exe','-std=gnu11','-O2','-ffunction-sections','-fuse-ld=lld','-Wl,/opt:ref','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include')]
    exe=out/'graph_inventory.exe';run('compile',flags+[HERE/'graph_inventory.c',*files,'-o',exe])
    records=[];inputs=[HERE/'graph_inventory.c',Path(__file__).resolve(),a.common.resolve(),*files]
    forward=sdk/'src/melee/ft/forward.h';text=forward.read_text();names=re.findall(r'^\s*(FtPart_\w+)\s*,',text,re.M)[:54];assert len(names)==53;names.append("unnamed_common_part53");inputs.append(forward)
    for family in ['link','young_link']:
        project=projects/family;fileset=[project/'fighter.dat',project/'costume.dat',project/'cpu_profile.fcp'];inputs+=fileset
        record=json.loads(run(family,[exe,family,*fileset[:2],a.common.resolve()]))
        record['semantics']={n:j for n,j in zip(names,record['semantic_part_to_joint'])}
        lines=fileset[2].read_text().splitlines();groups=['ground','air','ranged','smash','special','weapon','item'];record['cpu_family_counts']={g:sum(line.startswith(g+' ') for line in lines) for g in groups}
        record['cpu_profile_sha256']=sha(fileset[2])
        parameters=project/'fighter_parameters.fmp';inputs.append(parameters)
        record['owned_joint_defaults']={name:int(re.search(r'^int '+name+r' = (\d+)$',parameters.read_text(),re.M)[1]) for name in ['spin_effect_joint','bomb_holding_joint']}
        expected={'spin_effect_joint':24 if family=='link' else 26,'bomb_holding_joint':64 if family=='link' else 68}
        if record['owned_joint_defaults']!=expected:raise RuntimeError('normal import joint default mismatch')
        records.append(record)
    common_kind_sites=[]
    for source in sorted((sdk/'src/melee').rglob('*.c')):
        rel=source.relative_to(sdk/'src').as_posix()
        if any(token in rel for token in ['/ftLink/','/ftCLink/','/ftKirby/']):continue
        lines=source.read_text(encoding='utf-8').splitlines()
        found=[dict(line=i+1,text=line.strip()) for i,line in enumerate(lines) if re.search(r'\bFt_Kind_(?:Link|CLink)\b|\bft(?:Lk|Cl)_',line)]
        if found:common_kind_sites.append(dict(path=rel,sites=found,sha256=sha(source)))
    tables={};data=sdk/'src/melee/ft/ftdata.c';text=data.read_text();inputs.append(data)
    for m in re.finditer(r'^.*\b(\w+)\s*\[Ft_Kind_RuntimeCapacity\]\s*=\s*\{',text,re.M):
        entries,line=initializer(text,m.group(1))
        if len(entries)>20:tables[m.group(1)]={'line':line,'Link':entries[6],'Young Link':entries[20]}
    # Keep exact full table bodies and special row references in the frozen hash inventory.
    inputs += [sdk/'src'/s for s in ['melee/ft/ftparts.c','melee/ft/ftcpuattack.c','melee/ft/kinds/ftCommon/ftCo_0A01.c','melee/ft/kinds/ftLink/ftlink.c','melee/ft/kinds/ftCLink/ftclink.c','melee/ft/kinds/ftLink/ftlinkattackair.c','melee/ft/ftattacks4combo.c','melee/ft/kinds/ftCommon/ftCo_AttackS4.c','melee/ft/kinds/ftCommon/ftCo_Guard.c','melee/ft/kinds/ftCommon/ftCo_Wait.c','melee/ft/kinds/ftLink/ftlink_clone.c','melee/ft/kinds/ftCLink/ftclink_clone.c']]
    result={'snapshot':str(snap),'snapshot_sha256':sha(snap/'freeze.json'),'graphs':records,'callback_tables':tables,'common_named_kind_sites':common_kind_sites,'input_sha256':{str(x):sha(x) for x in inputs},'commands':commands,'limits':['normal selected costume only; no live model insertion or drawing','span_to_next_target_words is DAT target spacing, NOT an array length','runtime_slot_capacity16 is actual bounded owned import storage; layout masks define which slots are imported','no CPU gameplay or default activation claim']}
    verify();(out/'inventory.json').write_bytes((json.dumps(result,indent=2)+'\n').encode());print(out/'inventory.json')
if __name__=='__main__':main()
