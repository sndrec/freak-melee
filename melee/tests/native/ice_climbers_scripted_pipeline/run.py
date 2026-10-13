"""Normal scripted companion pipeline. Requires explicit rebuilt-tool/source handoff."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
import traceback
from asset_census import animation_census

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
ACTIVE_OUTPUT = None


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def write(path, text):
    path.write_text(text, encoding='utf-8', newline='\n')


def name_hash(name):
    result = 2166136261
    for byte in name.encode('ascii'):
        result = ((result ^ byte) * 16777619) & 0xffffffff
    return result


def scalar_bits(kind, value):
    if kind == 'float':
        return struct.unpack('>I', struct.pack('>f', float.fromhex(value) if 'p' in value.lower() else float(value)))[0]
    if kind == 'bool':
        assert value in ('true', 'false', '0', '1'), value
        return int(value in ('true', '1'))
    return int(value, 0) & 0xffffffff


def parameter_audit(source, rows, dat, top, accepted):
    word = lambda at: struct.unpack_from('>I', dat, 32 + at)[0]
    params = {row['name']: row for row in rows}
    assert len(params) == len(rows), 'Duplicate imported parameter'
    attributes = word(top + 4)
    table = word(top + 72)
    article_bases = {name: word(word(table + slot * 4) + 4) for slot, name in enumerate(
        ('It_Kind_IceClimber_Ice', 'It_Kind_IceClimber_Blizzard', 'It_Kind_IceClimber_GumStrings'))}
    facts = {None: accepted.FIGHTER_TUNING, 'It_Kind_IceClimber_Ice': accepted.ICE_TUNING,
             'It_Kind_IceClimber_Blizzard': accepted.BLIZZARD_TUNING}
    scope, checked, retail_checks = None, set(), 0
    for line in source.splitlines():
        if line.startswith('article '): scope = line.split()[1]
        match = re.fullmatch(r'(\s*)(attribute|parameter) (float|int|bool|byte) (\w+) (?:= )?(.+)', line)
        if not match: continue
        indent, form, kind, name, value = match.groups()
        owner = scope if indent else None
        key = (owner + '.' if owner else '') + name
        assert key not in checked and key in params, key
        checked.add(key)
        base = article_bases[owner] if owner else attributes
        if form == 'attribute':
            offset = int(value, 0)
            bits = dat[32 + base + offset] if kind in ('byte', 'bool') else word(base + offset)
        else:
            bits = scalar_bits(kind, value)
        actual_kind = 'int' if kind == 'byte' else kind
        assert params[key]['type'] == actual_kind and params[key]['bits'] == bits, (key, params[key], bits)
        # Accepted slice facts identify real retail tuning even if a source
        # accidentally retains its old literal fixture placeholder.
        if name in facts.get(owner, {}):
            expected = word(base + facts[owner][name])
            assert bits == expected, ('Retail tuning placeholder', key, bits, expected)
            retail_checks += 1
    assert checked == set(params), ('Unaccounted FMP fields', set(params) - checked)
    return {'all_declarations': len(checked), 'accepted_retail_tuning': retail_checks}


def source_census(source, member):
    assert re.search(r'(?m)^base ftData'+member.capitalize()+r' 341 26$', source), 'Wrong member base declaration'
    phases = re.findall(r'(?m)^    on state_(\d+)_(animation|interrupt|physics|collision):', source)
    assert len(phases) == len(set(phases)) == 96, 'Expected all 96 actual custom phases'
    assert not any(int(state) in (363, 366) for state, _ in phases), 'Original NULL rows must remain absent'
    hooks = set(re.findall(r'(?m)^    on (\w+):', source))
    required = {'cpu_capture_decision', 'cpu_pummel_special'} if member == 'popo' else {
        'cpu_prepare', 'cpu_initialize', 'cpu_spacing_update', 'cpu_stick_step',
        'cpu_stick_limit', 'cpu_drop_command', 'cpu_mode_update', 'cpu_output'}
    assert required <= hooks, ('CPU handoff incomplete', member, required - hooks)
    if member == 'nana':
        assert not {'special_s', 'special_air_s', 'special_hi', 'special_air_hi'} & hooks
    return {'phases': len(phases), 'hooks': sorted(hooks), 'required_cpu_hooks': sorted(required)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('tool', 'clang', 'iso', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--assembler', type=Path, default=HERE.parent / 'ice_climbers_activation/composition72/assemble.py')
    parser.add_argument('--prepare-only', action='store_true')
    args = parser.parse_args()
    tool, clang, iso, out, assembler = [value.resolve() for value in
        (args.tool, args.clang, args.iso, args.output, args.assembler)]
    if out.exists() or out.drive.upper() != 'A:': parser.error('Use a fresh isolated A: output')
    # script-build embeds live include paths. Pin the actual live sources and
    # binary; never describe this as a stale snapshot tool/header build.
    inputs = {str(path): sha(path) for folder in (ROOT/'src', ROOT/'extern/dolphin/include', ROOT/'tools')
              for path in folder.rglob('*') if path.is_file() and path.suffix in ('.c', '.h', '.py')}
    dependencies = ('ice_climbers_activation', 'ice_climbers_owned_pair', 'ice_climbers_composition',
        'ice_climbers_linked', 'ice_climbers_rope', 'ice_climbers_cpu_owned',
        'ice_climbers_asset_pipeline', 'zelda_sheik_owned_pair', 'sheik_clone_activation')
    for name in dependencies:
        inputs.update({str(path): sha(path) for path in (HERE.parent/name).rglob('*')
                       if path.is_file() and '__pycache__' not in path.parts})
    inputs.update({str(path): sha(path) for path in HERE.iterdir() if path.is_file()})
    inputs.update({str(path): sha(path) for path in (tool, clang, assembler)})
    inputs.update({str(path): sha(path) for path in clang.parent.iterdir() if path.is_file()})
    global ACTIVE_OUTPUT
    out.mkdir(parents=True)
    ACTIVE_OUTPUT = out
    write(out/'inputs.json', json.dumps(inputs, indent=2)+'\n')
    assembly = load(assembler, 'full_climbers_assembly')
    sources, census = {}, {}
    for member in ('popo', 'nana'):
        source = assembly.assemble(member)
        census[member] = source_census(source, member)
        assert re.search(r'(?m)^    behavior independent$', source), 'Final assembler must own independent declaration'
        sources[member] = source
        write(out/(member+'.fms'), source.rstrip()+'\n')
    if args.prepare_only:
        write(out/'prepare.json', json.dumps({'status': 'SOURCE PREPARATION ONLY', 'independent': True,
            'census': census, 'inputs': inputs}, indent=2)+'\n')
        return
    commands = []
    def run(command, directory, stage, setup=False):
        command = list(map(str, command)); launched = command
        if setup:
            batch = directory/(stage+'.cmd')
            write(batch, '@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" -vcvars_ver=14.44 >nul\nif errorlevel 1 exit /b 1\n'+subprocess.list2cmdline(command)+'\n')
            launched = ['cmd', '/d', '/c', str(batch)]
        result = subprocess.run(launched, cwd=ROOT, text=True, encoding='utf-8', capture_output=True,
                                timeout=300, creationflags=subprocess.CREATE_NO_WINDOW)
        log = directory/(stage+'.log'); write(log, result.stdout+result.stderr)
        commands.append({'stage': stage, 'command': command, 'exit': result.returncode, 'log_sha256': sha(log)})
        write(out/'progress.json', json.dumps(commands, indent=2)+'\n')
        if result.returncode: raise RuntimeError(stage+'\n'+(result.stdout+result.stderr)[-8000:])
        return result.stdout
    def cli(directory, stage, *words, setup=False):
        return run([tool, '--root', out/'store', *words], directory, stage, setup)
    def content_id(text):
        ids = set(re.findall(r'\b[0-9a-f]{64}\b', text)); assert len(ids)==1, text
        return ids.pop()
    sys.path.insert(0, str(HERE.parent/'ice_climbers_asset_pipeline'))
    asset = load(HERE.parent/'ice_climbers_asset_pipeline/run.py', 'asset_pipeline_reader')
    retail = {name: asset.archive(iso, name) for name in
        ('PlPp.dat', 'PlNn.dat', 'PlPpAJ.dat', 'PlNnAJ.dat', 'PlPpNr.dat', 'PlNnNr.dat')}
    common = out/'PlCo.dat'; common.write_bytes(asset.archive(iso, 'PlCo.dat'))
    common_dat = load(HERE.parent/'ice_climbers_asset_pipeline/audit.py', 'common_dat_reader').Dat(common.read_bytes())
    part_table = common_dat.word(common_dat.roots['ftLoadCommonData'] + 16)
    image = out/'portrait.png'; asset.portrait(image)
    sys.path.insert(0, str(ROOT/'tools'))
    from extract_native_animation import extract_clip
    sys.path.insert(0, str(HERE.parent/'ice_climbers_asset_pipeline'))
    raw_audit = load(HERE.parent/'ice_climbers_asset_pipeline/audit.py', 'copied_asset_audit')
    accepted = load(HERE.parent/'ice_climbers_composition/fixture.py', 'retail_tuning_facts')
    table_parser = load(HERE.parent/'sheik_clone_activation/audit.py', 'retail_table_parser')
    header = (ROOT/'src/melee/ft/native_fighter_runtime.h').read_text(encoding='utf-8')
    definition = header.split('typedef struct NativeFighterDefinition {',1)[1].split('} NativeFighterDefinition;',1)[0]
    callback_fields = sorted(set(re.findall(r'\(\*(\w+)\)', definition) + re.findall(
        r'\b(?:HSD_GObjEvent|HSD_GObjPredicate|Fighter_ItemEvent)\s+(\w+)\s*;', definition)))
    members, files, ids, bindings = {}, {}, [], []
    for member, kind_id, short, symbol in (('popo',10,'Pp','Popo'), ('nana',11,'Nn','Nana')):
        directory=out/member; directory.mkdir(); project=directory/'project'
        cli(directory,'clone','clone-stock',iso,symbol,project,'Owned scripted '+symbol)
        cli(directory,'inspect','inspect-project',project)
        files[symbol] = {name:(project/name).read_bytes() for name in ('fighter.dat','animation.dat','costume.dat')}
        cli(directory,'import','import-parameters',project,out/(member+'.fms'),project/'behavior.fms')
        imported_source=(project/'behavior.fms').read_text(encoding='utf-8')
        expected_articles=re.findall(r'(?m)^article (\w+) states (\d+):',sources[member])
        assert re.findall(r'(?m)^article (\w+) states (\d+):',imported_source)==expected_articles, 'Normal import dropped/changed owned article declarations'
        metadata=(project/'project.txt').read_text(encoding='utf-8')
        key=re.search(r'(?m)^code=(\S+)$',metadata)[1]
        assert re.search(r'(?m)^base='+symbol+r'$', metadata), 'Wrong normal constructor base'
        assert key==member+'_clone', 'Unexpected project code identity'
        cli(directory,'script-build','script-build',project,project/'behavior.fms',setup=True)
        generated=(project/'behavior.generated.c').read_text(encoding='utf-8')
        assert f'generated.code_key = "{key}";' in generated
        assert 'generated.source_kind = Ft_Kind_None;' in generated
        count,schema=re.search(r'count != (\d+) \|\| schema != (0x[\da-fA-F]+)ULL',generated).groups()
        rows=[]
        for line in (project/'fighter_parameters.fmp').read_text(encoding='utf-8').splitlines():
            if not line or line.startswith('#'): continue
            match=re.fullmatch(r'(float|int|bool) (\S+) = (\S+)',line); assert match,line
            kind,name,value=match.groups(); rows.append({'name':name,'type':kind,'bits':scalar_bits(kind,value)})
        assert len(rows)==int(count)
        dat=(project/'fighter.dat').read_bytes()
        dat_reader=raw_audit.Dat(dat); top=dat_reader.roots['ftData'+symbol]
        tuning=parameter_audit(sources[member],rows,dat,top,accepted)
        write(directory/'parameters.inc','static const uint32_t parameter_words[]={'+','.join('UINT32_C('+str(row['bits'])+')' for row in rows)+'};\n#define PARAMETER_SCHEMA '+schema+'ULL\n')
        descriptor=common_dat.word(part_table+kind_id*4)
        mapping=common_dat.word(descriptor+4)
        raw_map=list(common_dat.data[mapping:mapping+54])
        assert sum(part!=255 for part in raw_map)==40 and common_dat.word(descriptor+8)==49
        table_path=ROOT/f'src/melee/ft/kinds/ft{symbol}/ft{member}.c'
        table_name='ft'+short+'_Init_MotionStateTable'
        table=table_parser.arrays(table_path.read_text(encoding='utf-8'),r'\b('+table_name+r')\s*\[[^\]]*\]\s*=\s*\{')[table_name]
        assert len(table)==26
        base_dir=table_path.parent; catalog=base_dir/f'ft{member}_clone.c'
        (directory/'catalog.c').write_bytes(catalog.read_bytes())
        (directory/f'ft{member}_clone.h').write_bytes((base_dir/f'ft{member}_clone.h').read_bytes())
        names=set()
        for row in table: names.update(value for value in table_parser.split(row[1:-1])[3:] if value not in ('NULL','0'))
        names.update(re.findall(r'\.special_\w+\s*=\s*(\w+)',catalog.read_text(encoding='utf-8')))
        stock='#include <melee/ft/kinds/ftPopo/ftpopo.h>\n#include <melee/ft/kinds/ftNana/ftnana.h>\n#include <melee/ft/ftcamera.h>\n'
        stock+=''.join(f'#include <melee/ft/kinds/ftPopo/ftpopospecial{s}.h>\n' for s in ('n','s','hi','lw'))
        stock+='\n'.join(f'void {name}(HSD_GObj* g) {{ (void)g; CHECK(false); }}' for name in sorted(names))+'\n'
        stock+='MotionState '+table_name+'[]={\n'+',\n'.join(table)+'\n};\n'
        stock+=f'#define BASE_DEFINITION ft{symbol}Clone_Definition\n#define MEMBER_KIND Ft_Kind_{symbol}\n#define NANA {int(member=="nana")}\n'
        stock+='#define PART_COUNT 49\n#define BONE_COUNT 40\n#define ACTION_COUNT 321\n#define PHASE_STATES 26\n#define ARTICLE_COUNT 3\n'
        stock+='#define EXPECTED_SOURCE_KIND Ft_Kind_None\n'
        write(directory/'stock_table.inc',stock)
        write(directory/'expected_bones.inc','static const NativeFighterBone expected_bones[]={'+','.join('{'+str(index)+','+str(part)+'}' for index,part in enumerate(raw_map) if part!=255)+'};\n')
        articles=re.findall(r'(?m)^article (\w+) states (\d+):',sources[member])
        assert len(articles)==3 and len({name for name,_ in articles})==3
        article_metadata=''
        article_rows=[]
        for name,count in articles:
            block=sources[member].split('article '+name+' states '+count+':',1)[1].split('\narticle ',1)[0]
            local_fields=re.findall(r'(?m)^    state (int|float|bool) (\w+)(?:\[(\d+)\])?$',block)
            assert local_fields, 'Expected actual article-local state '+name
            article_metadata+='static const ExpectedState '+name+'_fields[]={\n'+',\n'.join(
                '{"'+field+'",NativeFighterState_'+kind.title()+','+(n or '1')+','+str(bool(n)).lower()+'}' for kind,field,n in local_fields)+'\n};\n'
            article_rows.append('{'+name+','+count+','+str(len(local_fields))+','+name+'_fields}')
        write(directory/'expected_articles.inc',article_metadata+'typedef struct {ItemKind kind;unsigned states,field_count;const ExpectedState* fields;} ExpectedArticle;\nstatic const ExpectedArticle expected_articles[]={\n'+',\n'.join(article_rows)+'\n};\n')
        write(directory/'callbacks.inc',''.join('VISIT_CALLBACK('+name+');\n' for name in callback_fields))
        write(directory/'required_host.inc','\n'.join('host.'+name+'=(__typeof__(host.'+name+'))unexpected;' for name in sorted(set(re.findall(r'host->(\w+) == 0',generated))))+'\n')
        fields=re.findall(r'(?m)^state (int|float|bool) (\w+)(?:\[(\d+)\])?$',sources[member])
        write(directory/'expected_state.inc','typedef struct {const char* name;unsigned type,count;bool array;} ExpectedState;\nstatic const ExpectedState expected_state[]={\n'+',\n'.join(
            '{"'+name+'",NativeFighterState_'+kind.title()+','+(count or '1')+','+str(bool(count)).lower()+'}' for kind,name,count in fields)+'\n};\n')
        exports=sorted((name_hash(name),result or 'void') for name,result in re.findall(r'(?m)^export func (\w+)\([^\n]*\)(?: -> (bool|int|float))?:',sources[member]))
        write(directory/'expected_exports.inc','typedef struct {uint32_t hash;unsigned type;} ExpectedExport;\nstatic const ExpectedExport expected_exports[]={\n'+',\n'.join(
            '{UINT32_C('+str(h)+'),NativeFighterReturn_'+result.title()+'}' for h,result in exports)+'\n};\n')
        cmd=[clang,'-std=gnu11','-O2','-ffp-contract=off','-fuse-ld=lld','-fms-runtime-lib=dll','-DMELEE_NATIVE=1','-D_DLL','-D_MT','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS',
             '-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(directory),'-I'+str(base_dir),HERE/'ownership.c',ROOT/'src/melee/ft/native_fighter_cpu.c',ROOT/'src/sysdolphin/baselib/dat.c','-o',directory/'ownership.exe']
        run(cmd,directory,'compile-ownership',setup=True)
        owned=json.loads(run([directory/'ownership.exe',project/'module.dll',common,project/'cpu_profile.fcp'],directory,'ownership'))
        package=content_id(cli(directory,'bundle','bundle-project',project,image,iso)); ids.append(package)
        verification=cli(directory,'verify','verify-package',package,iso)
        assert 'registered and released.' in verification
        rows_binding=re.findall(r'Member (\d+) \(([^)]+)\) animation binding: (\d+) own-rig clips, (\d+) borrowed clips',verification)
        assert len(rows_binding)==1,verification
        inventory=animation_census(dat_reader,raw_audit.bank(files[symbol]['animation.dat']),top,kind_id)
        binding=tuple(map(int,rows_binding[0][2:])); assert binding==(inventory['own_clips'],inventory['borrowed_clips']), (binding,inventory)
        bindings.append(binding)
        for name,data in files[symbol].items(): assert (project/name).read_bytes()==data,'Authoring mutated raw payload '+name
        members[member]={'package_id':package,'ownership':owned,'parameter_audit':tuning,'parameters':rows,
            'source_census':census[member],'animation_inventory':inventory,'retail_rig':{'parts':49,'semantic_map':raw_map},'verification':verification,'project_files':{path.name:sha(path) for path in project.iterdir() if path.is_file()}}
        write(out/'partial_members.json',json.dumps(members,indent=2)+'\n')
    serialized=raw_audit.audit(retail,files['Popo'],files['Nana'])
    family=content_id(cli(out,'compose-companion','compose-family','companion','Owned scripted Popo Nana',*ids))
    family_verify=cli(out,'verify-companion','verify-package',family,iso)
    assert 'registered and released.' in family_verify
    rows_binding=re.findall(r'Member (\d+) \(([^)]+)\) animation binding: (\d+) own-rig clips, (\d+) borrowed clips',family_verify)
    assert len(rows_binding)==2 and [(row[0],row[1]) for row in rows_binding]==[('0','main'),('1','partner')], family_verify
    assert [tuple(map(int,row[2:])) for row in rows_binding]==bindings,family_verify
    for path,expected in inputs.items(): assert sha(Path(path))==expected,'Live input changed: '+path
    receipt={'status':'PASS normal scripted companion console package pipeline; uninstalled, gameplay unproven',
        'independent':True,'family_id':family,'content_root':str(out/'store'),'cached_only':True,'isolated_accept_command':[str(tool),'--root',str(out/'store'),'accept',family,str(iso)],'family_verification':family_verify,'members':members,
        'retail_archive_sha256':{name:hashlib.sha256(data).hexdigest() for name,data in retail.items()},'common_archive_sha256':sha(common),
        'serialized_asset_audit':serialized,'commands':commands,'inputs':inputs,
        'outputs':{str(path.relative_to(out)):sha(path) for path in out.rglob('*') if path.is_file()},
        'limits':['DLL probe resolves real callbacks but never executes gameplay. Source comparisons remain in accepted component namespaces.',
            'Actual verify-package imports/registers/releases both companion assets; no match, companion coordination, lifecycle or rollback simulation is proved.',
            'Action tag counts include undecoded demo metadata. Genuine copied Popo/Taro retarget tags remain; no zero-borrowed assertion.',
            'Raw asset audit reuses the accepted production-output audit; no helper materialization or replacement constructor runs.',
            'Initial costume only. Results/intro demo payload/rendering acceptance remains separate.']}
    write(out/'receipt.json',json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({'family_id':family,'members':{name:value['ownership'] for name,value in members.items()},'bindings':bindings}))


if __name__=='__main__':
    try:
        main()
    except Exception as error:
        if ACTIVE_OUTPUT is not None:
            write(ACTIVE_OUTPUT/'failure.json', json.dumps({'status':'FAIL',
                'error':str(error),'traceback':traceback.format_exc(),
                'outputs':{str(path.relative_to(ACTIVE_OUTPUT)):sha(path)
                           for path in ACTIVE_OUTPUT.rglob('*') if path.is_file()}}, indent=2)+'\n')
        raise
