"""Actual source-alive admission, extracted without gameplay substitutes."""
import hashlib,importlib.util,re,subprocess
from pathlib import Path

def extract(root,out):
 root=Path(root);out=Path(out);path=root/'tests/native/script_form_feedback/extract.py'
 spec=importlib.util.spec_from_file_location('body',path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 p=root/'src/melee/ft/native_script_binding_properties.c';text=p.read_text()
 (out/'source_guards.inc').write_text('\n'.join(re.sub(r'\berror\b','observation_error',m.body(text,n)) for n in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout'])+'\n')
 # Exact production routing, descriptor admission and staged publication; all
 # unselected binding families are linked as explicit unavailable leaves.
 prefix=text[:text.index('static bool error')]
 (out/'observation_reads.inc').write_text(prefix+'\n'+'\n'.join(re.sub(r'\berror\b','observation_error',m.body(text,n)) for n in ['property','vector_component','nativeScriptBindings_Read','nativeScriptBindings_Write'])+'\n')
 hashes={str(p):hashlib.sha256(p.read_bytes()).hexdigest(),str(path):hashlib.sha256(path.read_bytes()).hexdigest()}
 p=root/'src/melee/ft/native_script_binding_fighter_items.c';text=p.read_text()
 prefix=text[:text.index('typedef struct ItemView')]
 record=text[text.index('typedef struct ItemView'):text.index('} ItemView;')+len('} ItemView;')]
 bodies='\n'.join(m.body(text,n) for n in ['error','caller_current','policy_current','policy_capture','item_current','item_capture','nativeScriptBindingFighterItems_Read','nativeScriptBindingFighterItems_Write'])
 (out/'pickup_policy.inc').write_text(prefix+record+re.sub(r'\berror\b','pickup_error',bodies)+'\n')
 hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest()
 p=root/'src/melee/ft/native_script_binding_item_observations.c';text=p.read_text()
 prefix=text[:text.index('static bool error')]
 bodies='\n'.join(m.body(text,n) for n in ['error','source','nativeScriptBindingItemObservations_Read'])
 bodies=re.sub(r'\berror\b','floor_observation_error',bodies);bodies=re.sub(r'\bsource\(','floor_observation_source(',bodies)
 (out/'item_observation_reads.inc').write_text(prefix+bodies+'\n')
 hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest()
 p=root/'src/melee/it/it_26B1.c'
 native=m.body(p.read_text(),'it_8026B47C')
 reference=subprocess.run(['git','show','1e9849431:src/melee/it/it_26B1.c'],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
 if native!=m.body(reference,'it_8026B47C'):raise RuntimeError('Native heal-amount reference body changed')
 (out/'native_heal_amount.inc').write_text(native+'\n')
 hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest()
 p=root/'src/melee/pl/plbonus.c'
 (out/'native_action_count.inc').write_text(m.body(p.read_text(),'pl_800386D8')+'\n')
 hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest()
 p=root/'src/melee/ft/kinds/ftCommon/ftCo_Guard.c'
 weight=root/'src/melee/it/it_26B1.c'
 (out/'native_shield_weight.inc').write_text(m.body(weight.read_text(),'itIsHeavy')+'\n'+m.body(weight.read_text(),'itGetHoldKind')+'\n'+m.body(p.read_text(),'ftCo_80092ED8')+'\n')
 hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest();hashes[str(weight)]=hashlib.sha256(weight.read_bytes()).hexdigest()
 p=root/'src/melee/mp/mplib.c';text=p.read_text()
 # Actual native low-byte lookup and record layouts, with controlled stage
 # material tables supplied by the fixture rather than full stage assets.
 start=text.index('struct mpLib_803BF248_t_x4 {');end=text.index('};',text.index('struct mpLib_803BF248_t {',start))+2
 (out/'native_floor_material.inc').write_text(text[start:end]+'\nstatic struct mpLib_803BF248_t_x4 material_values[2][20];\nstatic struct mpLib_803BF248_t_x4* material_tables[2][20];\nstatic struct mpLib_803BF248_t mpLib_803BF248[2];\n'+m.body(text,'mpLib_800569EC')+'\n')
 hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest()
 p=root/'src/melee/ft/ft_0892.c'
 (out/'native_action_metadata.inc').write_text(m.body(p.read_text(),'decode_action_metadata')+'\n')
 hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest()
 p=root/'src/melee/ft/native_script_binding_combat.c';text=p.read_text()
 invocation=m.body(text,'nativeScriptBindingCombat_Invoke')
 # Keep the exact selected entry prefix. All later combat operations are
 # explicitly unavailable in this cohort, not successful engine substitutes.
 invocation=invocation[:invocation.index('    if(semantic==NativeScriptBinding_DamageUpdateTimer')]+\
  '    (void)values;return false;\n}\n'
 prefix=text[:text.index('static bool error')]
 selected='\n'.join(m.body(text,name) for name in ['error','interrupt_cleanup'])+'\n'+invocation
 (out/'native_interrupt_binding.inc').write_text(prefix+re.sub(r'\berror\b','interruption_error',selected)+'\n')
 hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest()
 pieces=[]
 for relative,names in [('src/melee/ft/ftcommon.c',['ftCommon_8007DB58']),
                        ('src/melee/ft/ft_0881.c',['ft_80088770','ft_800887CC'])]:
  p=root/relative;text=p.read_text()
  reference=subprocess.run(['git','show','839072faf:'+relative],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
  for name in names:
   actual=m.body(text,name)
   if actual!=m.body(reference,name):raise RuntimeError('Native interruption reference body changed: '+name)
   pieces.append(actual)
  hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest()
 (out/'native_interrupt_engine.inc').write_text('\n'.join(pieces)+'\n')
 return hashes
