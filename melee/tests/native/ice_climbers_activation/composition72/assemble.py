"""Compose accepted owned phases plus explicit load/common policy, without building."""
import argparse, hashlib, importlib.util, json, re
from pathlib import Path
HERE=Path(__file__).resolve().parent
ACCEPTED=HERE.parents[1]/'ice_climbers_owned_pair'

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def restore_imports(text):
    maps=json.loads((HERE/'import_tuning.json').read_text(encoding='utf-8'))
    for name,digest in maps['provenance'].items():
        assert sha(HERE.parents[3]/name)==digest,'Frozen import-offset evidence changed: '+name
    lines=[];scope=None
    for line in text.splitlines():
        if line.startswith('article '):scope=line.split()[1]
        if line.startswith(('move ','func ','export func ')):scope=None
        match=re.fullmatch(r'(\s*)parameter (float|int|bool) (\w+) = (.+)',line)
        if match:
            indent,kind,name,default=match.groups()
            fields=maps['articles'].get(scope,{}) if indent else maps['fighter']
            if name in fields:line=indent+'attribute '+kind+' '+name+' '+str(fields[name])
        lines.append(line)
    return '\n'.join(lines)+'\n'

def cpu_fragment(member):
    inventory=json.loads((HERE/'cpu_inputs.json').read_text(encoding='utf-8'))
    directory=HERE.parents[3]/inventory['namespace']
    for name,digest in inventory['files'].items():
        assert sha(directory/name)==digest,'Frozen CPU input changed: '+name
    name='follower.fms' if member=='nana' else 'capture.fms'
    text=(directory/name).read_text(encoding='utf-8')
    marker='move FollowerPolicy:' if member=='nana' else 'move PrimaryCapturePolicy:'
    assert text.count(marker)==1,'Expected one complete CPU move'
    helpers,hooks=text.split(marker,1)
    parameters=(directory/'parameters.fms').read_text(encoding='utf-8') if member=='nana' else ''
    return parameters,helpers,hooks

def assemble(member):
    spec=importlib.util.spec_from_file_location('accepted_owned_pair',ACCEPTED/'compose.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    text=restore_imports(module.compose(member))
    old='    on load:\n        fighter.copy_ext_attributes()'
    assert text.count(old)==1,'Expected exactly one accepted load entry'
    decision=('family.retire_companion_on_sleep(true)' if member=='popo'
              else 'items.reject_healing_pickup(true)')
    text=text.replace(old,'    on load:\n        '+decision+'\n        fighter.copy_ext_attributes()',1)
    # Reuse complete accepted callback bodies, state/typed export ownership and
    # special-entry asymmetry. Only the formerly pending load flag is inserted.
    text=re.sub(r'^# Unregistered full motion candidate;[^\n]*\n','',text)
    text=text.replace('# Representable lifecycle subset. Popo sleep-peer retirement and Nana special\n# pickup filtering are exact pending generic metadata decisions, not flags hidden\n# in fighter-specific union storage. See contracts.md.\n',
                      '# Exact load policy uses generic shared flags; article registration is owned metadata.\n')
    at=text.index('\n')+1
    text=text[:at]+(HERE/'common_policy.fms').read_text(encoding='utf-8')+'\n'+text[at:]
    if member=='nana':
        # The accepted source fixture's FMP stage selected these actual raw Nana
        # cue alternatives. Publish that same owned policy in final declarations;
        # normal parameter import must not depend on a fixture-only override.
        for name,value in {'ice_launch_voice':130090,'blizzard_voice':0x1FC0F,'blizzard_loop':0x1FBEE}.items():
            text,count=re.subn(r'(?m)^parameter int '+name+r' = \d+$',
                              'parameter int '+name+' = '+str(value),text)
            assert count==1,'Missing unique member cue '+name
    parameters,helpers,hooks=cpu_fragment(member)
    start=text.index('move OwnedLifecycle:')
    text=text[:start]+helpers+'\n'+text[start:]
    start=text.index('move OwnedLifecycle:')
    end=re.search(r'(?m)^(?:move|article) ',text[start+len('move OwnedLifecycle:'):])
    assert end,'Expected next top-level move/article after lifecycle'
    boundary=start+len('move OwnedLifecycle:')+end.start()
    text=text[:boundary]+hooks+'\n'+text[boundary:]
    at=text.index('\n')+1
    text=text[:at]+parameters+'\nfighter:\n    behavior independent\n\n'+text[at:]
    states=re.findall(r'(?m)^state (?:int|float|bool) .+$',text)
    assert len(states)==len(set(states)),'Duplicate fighter state declaration'
    text=re.sub(r'(?m)^state (?:int|float|bool) .+\n','',text)
    at=text.index('func forget_ice(')
    text=text[:at]+'\n'.join(states)+'\n\n'+text[at:]
    return '# Complete authored decisions; independent package request, pending normal pipeline/runtime acceptance.\n'+text

def coverage(source,member):
    stock=json.loads((ACCEPTED/'stock_census.json').read_text(encoding='utf-8'))
    fighter=source.split('\narticle ',1)[0]
    hooks=re.findall(r'(?m)^    on (\w+):',fighter)
    assert len(hooks)==len(set(hooks)),'Duplicate fighter handler'
    phases=[]
    for row in stock['motion_tables']['ftPopo' if member=='popo' else 'ftNana']:
        for phase,callback in row['phases'].items():
            suffix={'animation':'animation','input':'interrupt','physics':'physics','collision':'collision'}[phase]
            name='state_%d_%s'%(row['id'],suffix)
            assert (callback!='NULL')==(name in hooks),'Exact stock phase mismatch '+name
            phases.append({'motion':row['id'],'phase':phase,'original':callback,'authored':name if name in hooks else None})
    tables={'ftData_OnLoad':'load','ftData_OnDeath':'death','ftKindCalcIndiviParamTable':'load_special_attrs',
            'ftData_OnItemPickupExt':'item_pickup_ext','ftData_OnItemDropExt':'item_drop_ext',
            'ftData_OnItemPickup':'item_pickup','ftData_OnItemDrop':'item_drop',
            'ftData_OnItemVisible':'item_visible','ftData_OnItemInvisible':'item_invisible',
            'ftData_OnKnockbackEnter':'knockback_enter','ftData_OnKnockbackExit':'knockback_exit',
            'ftData_SpecialN':'special_n','ftData_SpecialAirN':'special_air_n',
            'ftData_SpecialS':'special_s','ftData_SpecialAirS':'special_air_s',
            'ftData_SpecialHi':'special_hi','ftData_SpecialAirHi':'special_air_hi',
            'ftData_SpecialLw':'special_lw','ftData_SpecialAirLw':'special_air_lw'}
    callbacks=[]
    for table,hook in tables.items():
        original=stock['kind_tables'][table]['Popo' if member=='popo' else 'Nana']
        assert (original!='NULL')==(hook in hooks),'Exact stock entry/lifecycle mismatch '+hook
        callbacks.append({'table':table,'original':original,'authored':hook if hook in hooks else None})
    attributes=[];scope=None
    for line in source.splitlines():
        if line.startswith('article '):scope=line.split()[1]
        if line.startswith(('move ','func ','export func ')):scope=None
        m=re.fullmatch(r'(\s*)attribute (float|int|bool) (\w+) (\d+)',line)
        if m:attributes.append({'scope':scope if m[1] else None,'type':m[2],'name':m[3],'offset':int(m[4])})
    expected_cpu=(['cpu_capture_decision','cpu_pummel_special'] if member=='popo' else
                  ['cpu_prepare','cpu_initialize','cpu_spacing_update','cpu_stick_step','cpu_stick_limit','cpu_drop_command','cpu_mode_update','cpu_output'])
    assert all(name in hooks for name in expected_cpu),'Missing owned CPU policy'
    assert 'fighter:\n    behavior independent' in source,'Independent declaration must be authored'
    return {'cpu_hooks':expected_cpu,'independent_declaration':True,'hooks':hooks,'motion_phases':phases,'stock_entry_lifecycle':callbacks,
            'import_declarations':attributes,'exports':re.findall(r'(?m)^export func (\w+)',fighter)}

def main():
    p=argparse.ArgumentParser();p.add_argument('--member',choices=('popo','nana'),required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--baseline',type=Path,required=True)
    x=p.parse_args();out=x.output.resolve();out.mkdir(parents=True,exist_ok=False)
    source=assemble(x.member);(out/'candidate.fms').write_text(source,encoding='utf-8')
    # Source-only census; this does not claim generated ABI or import acceptance.
    states=re.findall(r'(?m)^state (int|float|bool) (\w+)(?:\[(\d+)\])?$',source)
    size=sum((int(n) if n else 1)*(1 if t=='bool' else 4) for t,_,n in states)
    params=re.findall(r'(?m)^\s*(?:parameter|attribute) (?:int|float|bool) (\w+)',source)
    phases=re.findall(r'(?m)^    on state_(\d+)_(animation|interrupt|physics|collision):',source)
    assert len(phases)==len(set(phases))==96,'Expected exact complete accepted custom phase coverage'
    inventory={'member':x.member,'fighter_state_declarations':len(states),'unaligned_state_bytes':size,
               'named_parameter_declarations':len(params),'article_kinds':re.findall(r'(?m)^article (\w+)',source),
               'functions':len(re.findall(r'(?m)^(?:export )?func ',source)),
               'stock_custom_phases':len(phases),
               'load_policy':'retire companion on Sleep' if x.member=='popo' else 'reject nonzero-healing pickup',
               'status':'source assembly only; no compilation/import/runtime acceptance'}
    dependencies=set(HERE.glob('*'))
    for name in ('ice_climbers_owned_pair','ice_climbers_composition','ice_climbers_linked','ice_climbers_rope'):
        dependencies.update((HERE.parents[1]/name).glob('*.fms'))
    tuning=json.loads((HERE/'import_tuning.json').read_text(encoding='utf-8'))
    dependencies.update(HERE.parents[3]/name for name in tuning['provenance'])
    dependencies.add(ACCEPTED/'compose.py');dependencies.add(ACCEPTED/'stock_census.json');dependencies.add(x.baseline.resolve()/'freeze.json')
    cpu=json.loads((HERE/'cpu_inputs.json').read_text(encoding='utf-8'))
    dependencies.update(HERE.parents[3]/cpu['namespace']/name for name in cpu['files'])
    manifest={str(p.resolve()):sha(p) for p in sorted(dependencies) if p.is_file()}
    inventory['inputs']=manifest;inventory['candidate_sha256']=sha(out/'candidate.fms')
    inventory['coverage']=coverage(source,x.member)
    (out/'assembly.json').write_text(json.dumps(inventory,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({k:v for k,v in inventory.items() if k not in ('inputs','coverage')},indent=2))

if __name__=='__main__':main()
