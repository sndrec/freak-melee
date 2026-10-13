"""Exact movement bodies and two bounded Fighter native blocks."""
import hashlib,importlib.util,re,subprocess,json
from pathlib import Path
def extract(root,out):
 hashes={}
 def load(rel,module):
  path=root/rel;hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
  spec=importlib.util.spec_from_file_location(module,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
 m=load('tests/native/script_form_feedback/extract.py','body')
 n=load('tests/native/script_item_scheduler/extract_scheduler.py','native')
 def source(rel):
  path=root/rel;hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest();return n.native_only(path.read_text())
 properties=source('src/melee/ft/native_script_binding_properties.c')
 (out/'source_guards.inc').write_text('\n'.join(re.sub(r'\berror\b','movement_guard_error',m.body(properties,name)) for name in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout'])+'\n')
 bodies=[]
 for file,names in [('native_fighter_held_motion.c',['nativeFighterHeldMotion_Select']),('ft_0D27.c',['ftCo_Mushroom_SelectAirResume']),('ft_08A1.c',['ft_8008A348']),('kinds/ftCommon/ftCo_AerialAction.c',['ftCo_AerialAction_CheckManual','ftCo_AerialAction_CheckAutomatic']),('kinds/ftCommon/ftCo_ItemParasolOpen.c',['ft_800CEF08_inline1','ftCo_ItemParasolOpen_Enter']),('kinds/ftCommon/ftCo_ItemParasolFallSpecial.c',['ftCo_800CF3C8'])]:
  text=source('src/melee/ft/'+file)
  bodies += [m.body(text,name) for name in names]
 fighter=source('src/melee/ft/fighter.c')
 def block(marker):
  start=fighter.index(marker);opening=fighter.index('{',start);depth=1;i=opening+1
  while depth:
   depth+=(fighter[i]=='{')-(fighter[i]=='}');i+=1
  return fighter[start:i]
 bodies.append('void movement_grounded(Fighter_GObj* gobj) { Fighter* fp=gobj->user_data;\n'+block('    if (fp->ground_or_air == GA_Ground) {')+'\n ++grounded_tail; }')
 bodies.append('void movement_timer(Fighter_GObj* gobj) { Fighter* fp=gobj->user_data;\n'+block('        if (fp->x2221_b4) {')+'\n ++timer_tail; }')
 prototypes='\n'.join(b[:b.index('{')].rstrip()+';' for b in bodies)
 (out/'native_movement.inc').write_text(prototypes+'\n'+'\n'.join(bodies)+'\n')
 references=[];baseline={}
 for file,names in [('native_fighter_held_motion.c',['nativeFighterHeldMotion_Select']),('ft_0D27.c',['ftCo_Mushroom_SelectAirResume']),('ft_08A1.c',['ft_8008A348']),('kinds/ftCommon/ftCo_AerialAction.c',['ftCo_AerialAction_CheckManual','ftCo_AerialAction_CheckAutomatic'])]:
  rel='src/melee/ft/'+file
  result=subprocess.run(['git','show','HEAD:'+rel],cwd=root,capture_output=True,timeout=30,creationflags=subprocess.CREATE_NO_WINDOW)
  if result.returncode:raise RuntimeError('Cannot obtain unchanged HEAD '+rel)
  baseline[rel]=hashlib.sha256(result.stdout).hexdigest()
  text=n.native_only(result.stdout.decode());(out/('head_'+Path(file).name)).write_text(text)
  for name in names:
   body=m.body(text,name);references.append(re.sub(r'\b'+name+r'\b','reference_'+name,body))
 (out/'head_reference.inc').write_text('\n'.join(references)+'\n')
 (out/'head_reference_proof.json').write_text(json.dumps(baseline,indent=2)+'\n')
 return hashes
