"""Actual live rebuilt clone/import/script-build/alternate-package pipeline."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

from compose import compose
from audit import helpers
from animation_inventory import inventory

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('tool','clang','iso','output'):
        p.add_argument('--'+name, type=Path, required=True)
    args=p.parse_args()
    tool, clang, iso, out=(x.resolve() for x in (args.tool,args.clang,args.iso,args.output))
    if out.drive.upper()!='A:' or out.exists():
        p.error('Use a fresh isolated output on A:')
    # This is deliberately a live-source gate. script-build embeds ROOT, so
    # freeze the exact headers/sources it can observe rather than mislabeling it
    # a previous frozen SDK run. Root must grant the rebuilt-tool handoff first.
    inputs={str(x):sha(x) for directory in (ROOT/'src',ROOT/'extern/dolphin/include')
        for x in directory.rglob('*') if x.is_file() and x.suffix in ('.h','.c')}
    inputs.update({str(tool):sha(tool),str(clang):sha(clang)})
    for directory in (HERE,*[HERE.parent/n for n in ('paired_form_composition','sheik_clone_activation',
            'sheik_chain_composition','sheik_script_composition','zelda_script_composition',
            'zelda_travel58_composition','zelda_din_cpu')]):
        inputs.update({str(x):sha(x) for x in directory.rglob('*') if x.is_file() and '__pycache__' not in x.parts})
    inputs[str(ROOT/'tools/extract_native_animation.py')]=sha(ROOT/'tools/extract_native_animation.py')
    inputs[str(HERE.parent/'fighter_script_bundle_roundtrip.py')]=sha(HERE.parent/'fighter_script_bundle_roundtrip.py')
    out.mkdir(parents=True)
    commands=[]
    def run(command,directory,stage,setup=False):
        command=list(map(str,command))
        launched=command
        if setup:
            batch=directory/(stage+'.cmd')
            batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" -vcvars_ver=14.44 >nul\nif errorlevel 1 exit /b 1\n'+subprocess.list2cmdline(command)+'\n',encoding='utf-8',newline='\n')
            launched=['cmd','/d','/c',str(batch)]
        r=subprocess.run(launched,cwd=ROOT,text=True,encoding='utf-8',capture_output=True,timeout=300,creationflags=subprocess.CREATE_NO_WINDOW)
        log=directory/(stage+'.log');log.write_text(r.stdout+r.stderr,encoding='utf-8',newline='\n')
        commands.append({'stage':stage,'command':command,'launched':launched,'exit':r.returncode,'log_sha256':sha(log)})
        if r.returncode: raise RuntimeError(stage+'\n'+(r.stdout+r.stderr)[-6000:])
        return r.stdout
    def cli(directory,stage,*words,setup=False):
        return run([tool,'--root',out/'store',*words],directory,stage,setup)
    def content_id(text):
        ids=set(re.findall(r'\b[0-9a-f]{64}\b',text))
        if len(ids)!=1: raise RuntimeError('Expected one content ID: '+text)
        return ids.pop()
    def verify_bindings(text,expected):
        rows=re.findall(r'Member (\d+) \(([^)]+)\) animation binding: (\d+) own-rig clips, (\d+) borrowed clips',text)
        assert len(rows)==len(expected), text
        for (_,_,own,borrowed), census in zip(rows,expected):
            assert int(own)==census['own_clips'] and int(borrowed)==census['borrowed_clips'], (rows,census)
        assert 'registered and released.' in text
        return [{'member':int(i),'key':key,'own_clips':int(own),'borrowed_clips':int(borrowed)} for i,key,own,borrowed in rows]
    sys.path.insert(0,str(ROOT/'tools'))
    from extract_native_animation import extract_clip
    sys.path.insert(0,str(HERE.parent))
    from fighter_script_bundle_roundtrip import portrait
    image=out/'portrait.png';portrait(image)
    common,common_root=extract_clip(iso,'PlCo.dat','ftLoadCommonData')
    word=lambda data,offset:struct.unpack_from('>I',data,32+offset)[0]
    part_table=word(common,common_root+16)
    parser=helpers()
    header=(ROOT/'src/melee/ft/native_fighter_runtime.h').read_text(encoding='utf-8')
    definition=header.split('typedef struct NativeFighterDefinition {',1)[1].split('} NativeFighterDefinition;',1)[0]
    callback_fields=sorted(set(re.findall(r'\(\*(\w+)\)',definition)+re.findall(
        r'\b(?:HSD_GObjEvent|HSD_GObjPredicate|Fighter_ItemEvent)\s+(\w+)\s*;',definition)))
    members={};ids=[]
    for member,kind_id,short,kind,folder,stem,actions,parts,bones,states,slots in (
        ('zelda',19,'Zd','Zelda','ftZelda','zelda',311,118,53,18,{'It_Kind_Zelda_DinFire':0,'It_Kind_Zelda_DinFire_Explode':1}),
        ('sheik',7,'Sk','Seak','ftSeak','seak',317,58,52,24,{'It_Kind_Seak_NeedleThrow':0,'It_Kind_Seak_NeedleHeld':1,'It_Kind_Seak_Vanish':2,'It_Kind_Seak_Chain':3})):
        directory=out/member;directory.mkdir();project=directory/'project'
        cli(directory,'clone','clone-stock',iso,member.capitalize(),project,'Owned '+member.capitalize()+' pair member')
        cli(directory,'inspect','inspect-project',project)
        candidate=directory/'candidate.fms';candidate.write_text(compose(member),encoding='utf-8',newline='\n')
        cli(directory,'import','import-parameters',project,candidate,project/'behavior.fms')
        metadata=(project/'project.txt').read_text(encoding='utf-8');key=re.search(r'(?m)^code=(\S+)$',metadata)[1]
        cli(directory,'script-build','script-build',project,project/'behavior.fms',setup=True)
        generated=(project/'behavior.generated.c').read_text(encoding='utf-8');assert 'generated.source_kind = Ft_Kind_None;' in generated
        assert f'generated.code_key = "{key}";' in generated
        count,schema=re.search(r'count != (\d+) \|\| schema != (0x[\da-fA-F]+)ULL',generated).groups()
        rows=[];parameter_words=[]
        for line in (project/'fighter_parameters.fmp').read_text(encoding='utf-8').splitlines():
            if not line or line.startswith('#'): continue
            match=re.fullmatch(r'(float|int|bool) (\S+) = (\S+)',line);assert match,line
            t,name,value=match.groups();bits=struct.unpack('>I',struct.pack('>f',float(value)))[0] if t=='float' else (int(value=='true') if t=='bool' else int(value,0)&0xffffffff)
            rows.append({'name':name,'type':t,'bits':bits});parameter_words.append(bits)
        assert int(count)==len(rows)
        dat,top=extract_clip(iso,'Pl'+short+'.dat','ftData'+kind)
        assert sha(project/'fighter.dat')==hashlib.sha256(dat).hexdigest()
        # Compare every declared retail tuning offset to the actual imported
        # words. Literal defaults are checked against the authored source too.
        params={r['name']:r for r in rows};article=None;checked=0
        for line in candidate.read_text(encoding='utf-8').splitlines():
            if line.startswith('article '): article=line.split()[1]
            attr=re.fullmatch(r'(\s*)attribute (float|int|bool) (\w+) (\d+)',line)
            literal=re.fullmatch(r'(\s*)parameter (float|int|bool) (\w+) = (.+)',line)
            if not attr and not literal: continue
            indent,t,name,value=(attr or literal).groups();name=(article+'.' if indent else '')+name
            if attr:
                base=word(dat,word(dat,word(dat,top+72)+slots[article]*4)+4) if indent else word(dat,top+4)
                bits=dat[32+base+int(value)] if t=='bool' else word(dat,base+int(value))
            else:
                bits=struct.unpack('>I',struct.pack('>f',float(value)))[0] if t=='float' else (int(value=='true') if t=='bool' else int(value,0)&0xffffffff)
            assert params[name]['type']==t and params[name]['bits']==bits,(name,params[name],bits)
            checked+=1
        assert checked==len(rows)
        descriptor=word(common,part_table+kind_id*4);mapping=word(common,descriptor+4)
        raw_map=list(common[32+mapping:32+mapping+54]);assert sum(x!=255 for x in raw_map)==bones and word(common,descriptor+8)==parts
        table_source=(ROOT/f'src/melee/ft/kinds/{folder}/ft{stem}.c').read_text(encoding='utf-8')
        table=parser.arrays(table_source,r'\b(ft'+short+r'_Init_MotionStateTable)\s*\[[^\]]*\]\s*=\s*\{')[f'ft{short}_Init_MotionStateTable']
        assert len(table)==states
        base_dir=ROOT/f'src/melee/ft/kinds/{folder}'
        catalog=base_dir/f'ft{member}_clone.c';catalog_header=base_dir/f'ft{member}_clone.h'
        (directory/'catalog.c').write_bytes(catalog.read_bytes());(directory/catalog_header.name).write_bytes(catalog_header.read_bytes())
        identifiers=set()
        for row in table:
            identifiers.update(x for x in parser.split(row[1:-1])[3:] if x not in ('NULL','0'))
        identifiers.update(re.findall(r'\.special_\w+\s*=\s*(\w+)',catalog.read_text(encoding='utf-8')))
        stock= f'#include <melee/ft/kinds/{folder}/ft{stem}.h>\n'+''.join(
            f'#include <melee/ft/kinds/{folder}/ft{stem}special{s}.h>\n' for s in ('n','s','hi','lw'))
        stock+='\n'.join(f'void {name}(HSD_GObj* g) {{ (void)g; CHECK(false); }}' for name in sorted(identifiers))+'\n'
        stock+=f'MotionState ft{short}_Init_MotionStateTable[] = {{\n'+',\n'.join(table)+'\n};\n'
        stock+=f'#define BASE_DEFINITION ft{member.capitalize()}Clone_Definition\n#define MEMBER_KIND Ft_Kind_{kind}\n#define SHEIK {int(member=="sheik")}\n'
        stock+=f'#define PART_COUNT {parts}\n#define BONE_COUNT {bones}\n#define ACTION_COUNT {actions}\n#define PHASE_STATES {states}\n#define ARTICLE_COUNT {len(slots)}\n'
        (directory/'stock_table.inc').write_text(stock,encoding='utf-8',newline='\n')
        (directory/'parameters.inc').write_text('static const uint32_t parameter_words[]={'+','.join('UINT32_C('+str(v)+')' for v in parameter_words)+'};\n#define PARAMETER_SCHEMA '+schema+'ULL\n',encoding='utf-8',newline='\n')
        (directory/'callbacks.inc').write_text(''.join('VISIT_CALLBACK('+x+');\n' for x in callback_fields),encoding='utf-8',newline='\n')
        (directory/'required_host.inc').write_text('\n'.join('host.'+x+'=(__typeof__(host.'+x+'))unexpected;' for x in sorted(set(re.findall(r'host->(\w+) == 0',generated))))+'\n',encoding='utf-8',newline='\n')
        compile_command=[clang,'-std=gnu11','-O2','-ffp-contract=off','-fuse-ld=lld','-fms-runtime-lib=dll','-DMELEE_NATIVE=1','-D_DLL','-D_MT','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(directory),'-I'+str(base_dir),HERE/'pipeline_ownership.c','-o',directory/'ownership.exe']
        run(compile_command,directory,'compile-ownership',setup=True)
        owned=json.loads(run([directory/'ownership.exe',project/'module.dll'],directory,'ownership'))
        package=content_id(cli(directory,'bundle','bundle-project',project,image,iso));ids.append(package)
        clips=inventory(iso,project,'Pl'+short+'AJ.dat',dat,top,kind_id,common,common_root,extract_clip)
        binding=verify_bindings(cli(directory,'verify','verify-package',package,iso),[clips])
        members[member]={'package_id':package,'metadata':metadata,'code_key':key,'parameters':rows,'ownership':owned,
            'animation_binding':binding,'animation_inventory':clips,'retail_rig':{'parts':parts,'bones':bones,'raw_semantic_map':raw_map},
            'project_files':{x.name:sha(x) for x in project.iterdir() if x.is_file()},
            'production_catalog_sha256':sha(catalog),'production_table_sha256':hashlib.sha256(table_source.encode()).hexdigest()}
    family=content_id(cli(out,'compose-alternate','compose-family','alternate','Owned Zelda Sheik source family',*ids))
    family_bindings=verify_bindings(cli(out,'verify-alternate','verify-package',family,iso),[m['animation_inventory'] for m in members.values()])
    for name,expected in inputs.items():
        if sha(Path(name))!=expected: raise RuntimeError('Live source changed during pipeline: '+name)
    receipt={'status':'PASS actual independent alternate family console pipeline; uninstalled, no gameplay',
        'family_id':family,'family_animation_bindings':family_bindings,'members':members,'commands':commands,'inputs':inputs,
        'outputs':{str(x.relative_to(out)):sha(x) for x in out.rglob('*') if x.is_file()},
        'limits':['Actual content-tool registration validates imported member rigs/animations/articles/poses and releases both runtime slots.',
                  'The DLL probe checks actual production base metadata and every callback pointer; it does not instantiate fighters or invoke gameplay.',
                  'CPU profile bytes are independently imported by clone-stock and hashed; original policy comparison remains in the accepted source namespaces.',
                  'Initial cloned costume only is packaged; earlier five-costume retail census is separate evidence.',
                  'Demo metadata is copied; demo payloads listed missing_demo_clips are absent and runtime action-only import does not decode demo bank. This remains a versus results/intro asset closure gap.',
                  'No alternate transfer/gameplay/render/install or Workshop family editing acceptance.',
                  'Historical probe README mislabeled msid222 Bowser branch as forward throw: actual source is down throw (forward219); root is correcting raw fallback using existing immutable animation family.']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'family_id':family,'ownership':{n:v['ownership'] for n,v in members.items()},'bindings':family_bindings}))


if __name__=='__main__':main()
