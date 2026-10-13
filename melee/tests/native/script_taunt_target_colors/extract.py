"""Actual guard/native reference extraction with explicit test observation hook."""
import hashlib,importlib.util,re
from pathlib import Path
def extract(root,out):
 root=Path(root);out=Path(out);hashes={}
 path=root/'tests/native/script_form_feedback/extract.py';spec=importlib.util.spec_from_file_location('target_body',path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 path=root/'tests/native/script_kinetics/extract_primitives.py';spec=importlib.util.spec_from_file_location('native_branches',path);branches=importlib.util.module_from_spec(spec);spec.loader.exec_module(branches)
 hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 def source(relative):
  path=root/relative;hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest();return path.read_text()
 text=source('src/melee/ft/native_script_binding_properties.c')
 guards=[re.sub(r'\berror\b','target_guard_error',m.body(text,name)) for name in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout']]
 marker=guards[1].index('{')+1;guards[1]=guards[1][:marker]+'\n    target_guard_probe(context);'+guards[1][marker:]
 (out/'source_guards.inc').write_text('\n'.join(guards)+'\n')
 actual=m.body(text,'nativeScriptBindings_SourceAlive')
 observed=actual[:actual.index('{')+1]+'\n    target_guard_probe(context);'+actual[actual.index('{')+1:]
 (out/'properties.c').write_text('#include "fixture.h"\n'+text.replace(actual,observed))
 text=source('src/melee/ft/native_fighter_target.c')
 native=[m.body(text,name) for name in ['nativeFighterTarget_Position','nativeFighterTarget_Airborne','nativeFighterTarget_IsFighter']]
 native.insert(0,m.body(source('src/melee/ft/ftlib.c'),'ftLib_8008741C'))
 native += [m.body(source('src/melee/ft/ftlib.c'),'ftLib_800870BC'),m.body(source('src/melee/ft/native_fighter_target.c'),'nativeFighterTarget_ControllerColor')]
 native += [m.body(source('src/melee/ft/kinds/ftCommon/ftCo_AppealS.c'),'ftCo_800DEAE8')]
 native += [m.body(source('src/melee/ft/ftdata.c'),'ftData_80085FD4')]
 (out/'native_target.inc').write_text('\n'.join(native)+'\n')
 # Unsupported sibling state operations must not become successful test leaves.
 unavailable=[]
 families={
  'src/melee/ft/ftcollision_ground.c':['ft_80082888','ft_80082708','ft_800827A0'],
  'src/melee/ft/ft_081B.c':['ft_800824A0','ft_80081D0C'],
  'src/melee/ft/ft_08A1.c':['ft_8008A348'],
  'src/melee/ft/kinds/ftCommon/ftCo_Fall.c':['ftCo_Fall_Enter'],
  'src/melee/ft/kinds/ftCommon/ftCo_FallAerial.c':['ftCo_FallAerial_Enter'],
  'src/melee/ft/kinds/ftCommon/ftCo_FallSpecial.c':['ftCo_80096900','ftCo_800969D8'],
  'src/melee/ft/kinds/ftCommon/ftCo_Landing.c':['ftCo_Landing_Enter','ftCo_Landing_Enter_Basic','ftCo_LandingFallSpecial_Enter'],
  'src/melee/ft/kinds/ftCommon/ftCo_LandingAir.c':['ftCo_LandingAir_EnterWithMsidLag'],
 }
 for file,names in families.items():
  text=branches.native_only(source(file))
  for name in names:
   signature=m.body(text,name).split('{',1)[0]
   unavailable.append(signature+'{ abort(); }')
 (out/'state_absent.inc').write_text('\n'.join(unavailable)+'\n')
 return hashes
