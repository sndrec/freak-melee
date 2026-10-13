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
 text=source('src/melee/ft/kinds/ftCommon/ftCo_AppealS.c')
 actual=m.body(text,'ftCo_AppealS_IASA')
 # Select the original nonnative branch for the independent native-order
 # reference. Its input leaves are controlled; source lifetime is tested in
 # the actual MELEE_NATIVE branch separately.
 reference=actual.replace('ftCo_AppealS_IASA','reference_taunt_inputs').replace('MELEE_NATIVE','SCRIPT_REFERENCE_CONSOLE_BRANCH')
 checks=[('ftCo_SpecialS.c','ftCo_SpecialS_CheckInput'),('ftCo_Attack100.c','ftCo_Attack100_CheckInput'),('ftCo_Attack100.c','ftCo_800D6824'),('ftCo_Attack100.c','ftCo_800D68C0'),('ftCo_Catch.c','ftCo_Catch_CheckInput'),('ftCo_AttackS4.c','ftCo_AttackS4_CheckInput'),('ftCo_AttackHi4.c','ftCo_AttackHi4_CheckInput'),('ftCo_AttackLw4.c','ftCo_AttackLw4_CheckInput'),('ftCo_AttackS3.c','ftCo_AttackS3_CheckInput'),('ftCo_AttackHi3.c','ftCo_AttackHi3_CheckInput'),('ftCo_AttackLw3.c','ftCo_AttackLw3_CheckInput'),('ftCo_Attack1.c','ftCo_Attack1_CheckInput'),('ftCo_Escape.c','ftCo_80099794'),('ftCo_Guard.c','ftCo_80091A4C')]
 leaves=[]
 for index,(file,name) in enumerate(checks):
  signature=m.body(source('src/melee/ft/kinds/ftCommon/'+file),name).split('{',1)[0]
  leaves.append(signature+'{ return input_check(gobj,'+str(index)+'); }')
 (out/'interrupts.inc').write_text('\n'.join(leaves)+'\n'+actual+'\n'+reference+'\n')
 # Preserve the substantive new kinetic query entry exactly; sibling kinetic
 # operations explicitly reject in this bounded fixture instead of gaining
 # fake physics/animation implementations.
 kinetic=source('src/melee/ft/native_script_binding_kinetics.c')
 query=m.body(kinetic,'nativeScriptBindingKinetics_Invoke').split('    (void)count;',1)[0]+'    return false;\n}\n'
 query=m.body(kinetic,'error')+'\n'+query
 query+='\n'+branches.body(source('src/melee/ft/ftdata.c'),'ftData_80085E50')+'\n'+m.body(source('src/melee/lb/lbanim.c'),'lbAnim_8001E8F8')+'\n'
 (out/'clip_query.c').write_text('#include <melee/ft/native_script_bindings.h>\n#include <melee/ft/native_script_expression.h>\n#include <melee/ft/native_source_package.h>\n#include <melee/ft/ftdata.h>\n#include <melee/lb/lbanim.h>\n#include <sysdolphin/baselib/gobj.h>\n#include <stdio.h>\n'+query)
 return hashes
