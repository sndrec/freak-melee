"""Actual stage orchestration and source guard bodies; geometry remains controlled."""
import hashlib,importlib.util,re,json,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('stage_body',HERE.parent/'script_form_feedback/extract.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
def extract(root,out):
 hashes={}
 def read(name):
  p=root/name;hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest();return p.read_text()
 props=read('src/melee/ft/native_script_binding_properties.c')
 guards='\n'.join(module.body(props,n) for n in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout'])
 guards=re.sub(r'\berror\b','stage_guard_error',guards)
 boundary=read('tests/native/script_fighter_scheduler/boundary.c')
 boundary=boundary.replace(module.body(boundary,'nativeScriptBindings_SourceAlive'),'')
 for name in ['mpIsland_8005AC14','mpIsland_8005ACE8']:
  boundary=boundary.replace(module.body(boundary,name),'')
 (out/'boundary.c').write_text('#include <melee/ft/native_source_package.h>\n#include <melee/ft/ftdat.h>\n'+boundary+'\n'+guards+'\n')
 coll=read('src/melee/mp/mpcoll.c');lib=read('src/melee/mp/mplib.c');vector=read('src/melee/lb/lbvector.c')
 names=['mpCollPrev','clamp_above','clamp_below','mpCollCheckBounding','mpColl_80041EE4','mpColl_80042C58','Vec2_Interpolate','mpCollInterpolateECB','mpColl_80043268','mpCollEnd_inline2','mpCollEnd_inline','mpCollEnd','mpColl_80043754','inline0','mpColl_8004730C']
 bodies=[module.body(coll,n) for n in names]
 bodies += [module.body(lib,'mpCheckAllRemap'),module.body(vector,'lbVector_Diff'),module.body(vector,'lbVector_Add'),module.body(read('src/melee/lb/lbclear.c'),'memzero')]
 bodies += [module.body(read('src/melee/ft/ftcommon.c'),'ftCommon_UnlockECB')]
 damage=read('src/melee/ft/kinds/ftCommon/ftCo_Damage.c')
 bodies += [module.body(damage,n) for n in ['ftCo_ScaleBy154','ftCo_Damage_CalcKnockbackLevel','ftCo_8008E984']]
 bodies += [module.body(read('src/melee/it/item.c'),'Item_8026AE60')]
 (out/'native_stage.inc').write_text('\n'.join(bodies)+'\n')
 terrain=[module.body(lib,n) for n in ['mpCheckFloor','mpLib_80054ED8']]
 terrain += [module.body(lib,n) for n in ['mpLineGetNext','mpLineGetPrev','mpLineGetV1Pos','mpLineGetV0Pos','mpLineGetKind','mpLineGetFlags','mpLib_8004DD90_Floor','sqrtf_store','mpLib_80056C54']]
 island_path='src/melee/mp/mpisland.c';island_text=read(island_path)
 terrain += [module.body(island_text,n) for n in ['mpIsland_8005AB54','mpIsland_8005AC8C','mpIsland_8005AC14','mpIsland_8005ACE8']]
 pinned=subprocess.run(['git','show','762ce905a:'+island_path],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
 names=['mpIsland_8005AB54','mpIsland_8005AC14','mpIsland_8005ACE8']
 reference='\n'.join(module.body(pinned,n) for n in names)
 for name in names:reference=re.sub(r'\b'+name+r'\b','reference_'+name,reference)
 (out/'island_reference.inc').write_text(reference+'\n')
 (out/'island_reference.json').write_text(json.dumps({'commit':'762ce905a','body_sha256':hashlib.sha256(reference.encode()).hexdigest(),'alterations':['function-name and inter-function-call renames only']},indent=2)+'\n')
 cpu=read('src/melee/ft/kinds/ftCommon/ftCo_0A01.c')
 reference_cpu=subprocess.run(['git','show','76ccf8a70:src/melee/ft/kinds/ftCommon/ftCo_0A01.c'],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
 floor_reference='\n'.join(module.body(reference_cpu,n) for n in ['ftCo_800A1B38','ftCo_800A0FB0'])
 floor_reference=floor_reference.replace('ftCo_800A1B38','original_cpu_floor_excluded').replace('ftCo_800A0FB0','original_cpu_floor_segment')
 (out/'cpu_floor_reference.inc').write_text(floor_reference+'\n')
 (out/'cpu_floor_reference.json').write_text(json.dumps({'commit':'76ccf8a70','sha256':hashlib.sha256(floor_reference.encode()).hexdigest(),'alterations':['entry and internal predicate names only']},indent=2)+'\n')
 terrain += [module.body(cpu,n) for n in ['ftCo_800A1F98','ftCo_800A5944','ftCo_CpuIslandBlocked']]
 terrain += [module.body(read('src/melee/it/ithitbox.c'),'it_80275870'),module.body(read('src/melee/it/it_26B1.c'),'it_8026C1B4')]
 for path,name in [('grbigblue','grBigBlue_801EF844'),('grinishie1','grInishie1_801FCAAC'),('grcorneria','grCorneria_801E2D90'),('grvenom','grVenom_80206D10')]:
  terrain.append(module.body(read('src/melee/gr/'+path+'.c'),name))
 (out/'native_terrain.inc').write_text('\n'.join(terrain)+'\n')

 # Pin the reference caller and authored consumers even though the cohort
 # compares the shared sequence rather than invoking stock-specific policies.
 for p in ['tools/native_fighter_script_collision.c','tools/native_fighter_script_primitives.c','tools/native_fighter_item_terrain.c','src/melee/ft/kinds/ftNess/ftnessattackhi4.c','examples/sheik_full.fms','examples/link_full.fms','examples/young_link_full.fms','examples/yoshi_wip.fms','examples/donkey_wip.fms']:read(p)
 # Read-only registry audit inputs; these callbacks are not executed by this
 # controlled geometry cohort. Actor-contact 1/2/3 is deliberately rejected.
 for p in (root/'src/melee/gr').glob('*.c'):
  text=p.read_bytes()
  if b'mpJointSetCb' in text or b'x0_gobj' in text:
   hashes[str(p)]=hashlib.sha256(text).hexdigest()
 return hashes
