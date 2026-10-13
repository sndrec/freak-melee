"""Actual predicates/entries, shared actor fixture, and pinned healthy entry reference."""
import hashlib,importlib.util,re,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
BASE=HERE.parent/'script_normal_attack_primitives'
spec=importlib.util.spec_from_file_location('ground_actor_substrate',BASE/'extract.py')
reference=importlib.util.module_from_spec(spec);spec.loader.exec_module(reference)
FUNCTIONS={
 'Catch':['ftCo_Catch_CheckInput','fn_800D8E94'],
 '0D8E':['fn_800D952C'],
 'AppealS':['ftCo_800DE9B8','ftCo_800DE9D8','ftCo_800DEA28'],
 'ItemThrow':['ftCo_800951D0','getAnimSpeed','ftCo_800958FC','ftCo_80094EA4'],
 'Guard':['inlineA0','ftCo_80091A4C','ftCo_800923B4','ftCo_800939B4'],
 'SpecialS':['doEnter','ftCo_SpecialS_CheckInput'],
 'Attack100':['ftCo_Attack100_CheckInput','ftCo_800D6824','ftCo_800D68C0'],
 'AttackS4':['checkLStick','checkItemThrow','ftCo_AttackS4_CheckInput'],
 'AttackHi4':['checkLStick','doEnter','ftCo_AttackHi4_CheckInput'],
 'AttackLw4':['checkLStick','doEnter','ftCo_AttackLw4_CheckInput'],
 'AttackS3':['decideAngle','ftCo_AttackS3_CheckInput'],
 'AttackHi3':['doEnter','ftCo_AttackHi3_CheckInput'],
 'AttackLw3':['ftCo_AttackLw3_CheckInput'],
 'Attack1':['ftCo_Attack1_CheckInput'],
 'Jump':['ftCo_Jump_GetInput','ftCo_Jump_CheckInput'],
 'KneeBend':['ftCo_KneeBend_Enter'],
 'Dash':['ftCo_Dash_Enter','ftCo_Dash_CheckInput'],
 'Squat':['ftCo_Squat_CheckInput','ftCo_Squat_Enter','ftCo_800D5FB0'],
 'Turn':['ftCo_800C97A8','ftCo_Turn_Enter','ftCo_Turn_Enter_Basic','ftCo_Turn_Enter_Smash','ftCo_Turn_CheckInput'],
 'Walk':['ftCo_Walk_Enter','ftCo_Walk_CheckInput'],
}
REFERENCE='b9ec6b132325f7b5b4ba45d8f62537265158528e'
HEAVY_REFERENCE='6e985dc6a'
def extract(root,out):
 hashes=reference.extract(root,out);bodies=[];includes=[]
 for unit,names in FUNCTIONS.items():
  path=root/f'src/melee/ft/kinds/ftCommon/ftCo_{unit}.c'
  text=reference.reference.reference.reference.native_only(path.read_text());hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
  includes+=re.findall(r'^#include .+$',text,re.M)
  rename={name:'Ground'+unit+'_'+name for name in names if name in ['checkLStick','checkItemThrow','decideAngle','doEnter','ftCo_Squat_Enter']}
  # Shared static functions already extracted by the normal-attack substrate.
  rename.update({'decideFighter':unit+'_decideFighter','decideAttack11':'Attack1_decideAttack11','doAttack12':'Attack1_doAttack12','doAttack13':'Attack1_doAttack13'})
  for name in names:
   value=reference.reference.body(text,name)
   value=re.sub(r'^(static\s+)?(inline\s+)?','',value)
   for old,new in rename.items():value=re.sub(r'\b'+old+r'\b',new,value)
   bodies.append(value)
 # Generic side-special continuation dependencies execute unchanged native
 # facing, source friction selection and the floor-surface projection. Only
 # the lowest stage material coefficient is a controlled engine leaf.
 for relative,names in [('src/melee/ft/ftcommon.c',['ftCommon_UpdateFacing','ftCommon_UnlockECB']),('src/melee/ft/ftmovement.c',['ft_GetGroundFrictionMultiplier']),('src/melee/mp/mpcoll.c',['mpColl_8004CA6C'])]:
  path=root/relative;text=reference.reference.reference.reference.native_only(path.read_text());hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
  for name in names:bodies.append(reference.reference.body(text,name))
 path=root/'src/melee/ft/ftwalkcommon.c';text=reference.reference.reference.reference.native_only(path.read_text());hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 includes+=re.findall(r'^#include .+$',text,re.M)
 for name in ['ftWalkCommon_GetWalkType_800DFBF8_fake','ftWalkCommon_800DFC70','ftWalkCommon_800DFCA4']:
  bodies.append(re.sub(r'^(static\s+)?(inline\s+)?','',reference.reference.body(text,name)))
 prototypes=[x[:x.index('{')].rstrip()+';' for x in bodies]
 actual=out/'actual_attack.h';actual.write_text(actual.read_text()+'\n'+'\n'.join(prototypes+bodies)+'\n')
 path=out/'boundary.c';s=path.read_text()
 # Reuse actual source property guards, and send just these observations to the new unit.
 old=reference.reference.body(s,'nativeScriptBindings_Read')
 new='bool nativeScriptBindings_Read(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* s,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){return nativeScriptBindingGroundControls_Read(c,id,s,n,r,d);}'
 s=s.replace(old,new)
 # Actual runtime explicitly supplies no callback-bearing native definition
 # for ordinary source packages. Keep that fact real in guard selection.
 runtime=root/'src/melee/ft/native_fighter_runtime.c';text=runtime.read_text();hashes[str(runtime)]=hashlib.sha256(runtime.read_bytes()).hexdigest()
 old=reference.reference.body(s,'nativeFighterRuntime_Definition')
 s=s.replace(old,reference.reference.body(text,'nativeFighterRuntime_Definition'))
 path.write_text('#include <melee/ft/native_script_binding_ground_controls.h>\n'+'\n'.join(dict.fromkeys(includes))+'\n'+s)
 # Record the unchanged pre-guard revision. Only healthy comparison bodies run.
 frozen=[]
 for unit,name in [('AttackS3','decideAngle'),('AttackHi3','doEnter'),('AttackHi4','doEnter'),('AttackLw4','doEnter'),('Dash','ftCo_Dash_Enter'),('Squat','ftCo_Squat_Enter'),('Turn','ftCo_Turn_Enter'),('Turn','ftCo_Turn_Enter_Smash')]:
  relative=f'src/melee/ft/kinds/ftCommon/ftCo_{unit}.c'
  original=subprocess.run(['git','show',REFERENCE+':'+relative],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
  text=reference.reference.reference.reference.native_only(original)
  value=re.sub(r'^(static\s+)?(inline\s+)?','',reference.reference.body(text,name))
  value=re.sub(r'\b'+name+r'\b','Frozen'+unit+'_'+name,value)
  frozen.append(value)
 relative='src/melee/ft/ftwalkcommon.c'
 original=subprocess.run(['git','show',REFERENCE+':'+relative],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
 value=reference.reference.body(reference.reference.reference.reference.native_only(original),'ftWalkCommon_800DFCA4').replace('ftWalkCommon_800DFCA4','FrozenWalk_Enter')
 frozen.append(value)
 # Heavy-item selection/entry has its own pre-change baseline. The same engine
 # leaves are used on both sides; callback and asset behavior are not inferred.
 relative='src/melee/ft/kinds/ftCommon/ftCo_ItemThrow.c'
 original=subprocess.run(['git','show',HEAVY_REFERENCE+':'+relative],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
 text=reference.reference.reference.reference.native_only(original)
 names=['getAnimSpeed','ftCo_800958FC','ftCo_80094EA4']
 heavy=[]
 for name in names:
  value=re.sub(r'^(static\s+)?(inline\s+)?','',reference.reference.body(text,name))
  for old in names:value=re.sub(r'\b'+old+r'\b','HeavyReference_'+old,value)
  heavy.append(value)
 frozen += [value[:value.index('{')].rstrip()+';' for value in heavy]+heavy
 (out/'heavy_reference.inc').write_text('\n'.join(heavy)+'\n')
 (out/'heavy_reference_revision.txt').write_text(HEAVY_REFERENCE+'\n')
 (out/'actual_attack.h').write_text(actual.read_text()+'\n'+'\n'.join(frozen)+'\n')
 (out/'reference_revision.txt').write_text(REFERENCE+'\n')
 return hashes
