"""Actual capture scheduler, owned source VM, and native grab arming."""
import argparse,hashlib,importlib.util,json,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];PRE=HERE.parent/'script_fighter_presentation_scheduler'
def module(name,path):
 s=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: required')
 out.mkdir(parents=True);ex=module('scheduler_extract',PRE/'extract.py');hashes=ex.extract(ROOT,out/'actual_presentation.h');body=ex.reference.reference.body
 original=PRE/'boundary.c';boundary=out/'boundary.c';text=original.read_text();text=text.replace(body(text,'nativeFighterRuntime_SourcePackage'),'').replace(body(text,'ftParts_GetBoneIndex'),'').replace(body(text,'nativeFighterRuntime_Definition'),'');text=text.replace(r'#include <stdlib.h>\n#include <stdio.h>\n#include <stdarg.h>','#include <stdlib.h>\n#include <stdio.h>\n#include <stdarg.h>');text=text.replace('presentation_bank','capture_package.compiler.bank');text=text.replace('#include "boundary.h"','#include "boundary.h"\n#include <melee/ft/native_source_package.h>\nextern NativeSourcePackage capture_package;');boundary.write_text(text)
 unavailable=out/'unavailable.c';unavailable.write_text((PRE/'unavailable.c').read_text().replace('CALL(nativeScriptBindingSetup_Invoke)','').replace('CALL(nativeScriptBindingCombat_Invoke)','').replace('CALL(nativeScriptBindingState_Invoke)',''))
 # Keep physical pools/context/position-retirement behavior, but route new
 # capture properties through their actual production bindings.
 b=boundary.read_text()
 # This fixture owns its jump/pose leaves; the reused presentation boundary
 # now supplies those too for its common-policy cohort. Keep one definition.
 for fn in ['HSD_Randi','ftCo_8009E7B4','ftAnim_8006EBE8','ftAnim_8006EBA4','ftCommon_8007D5D4','ftCo_JumpAerial_Begin']:
  b=b.replace(body(b,fn),'')
 b=b.replace('struct ftCommonData* p_ftCommonData=&policy_common;','')
 b=b.replace('if(n||s!=NativeScriptBinding_PositionX||!nativeScriptBindings_SourceAlive(c))return false;',
  'if(!n && ((s>=NativeScriptBinding_CaptureGroundLink && s<=NativeScriptBinding_CaptureEnclosureEscapeInvincibilityFrames)||s==NativeScriptBinding_CaptureVictimFrame||s==NativeScriptBinding_CaptureVictimAnimationFamily||s==NativeScriptBinding_CaptureVictimFreezeState))return nativeScriptBindingCapture_Read(c,s,out,d);if(!n && s==NativeScriptBinding_Airborne && nativeScriptBindings_SourceAlive(c)){*out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=((Fighter*)((HSD_GObj*)c->source)->user_data)->ground_or_air==GA_Air};return true;}if(n||s!=NativeScriptBinding_PositionX||!nativeScriptBindings_SourceAlive(c))return false;')
 b=b.replace('if(n||s!=NativeScriptBinding_PositionX||in->type!=NativeScriptType_F32||!nativeScriptBindings_SourceAlive(c))return false;',
  'if(!n && (s==NativeScriptBinding_CaptureGroundLink||s==NativeScriptBinding_CaptureVictimVisible||(s>=NativeScriptBinding_CaptureTimerDamageMultiplier&&s<=NativeScriptBinding_CaptureEnclosureEscapeInvincibilityFrames)))return nativeScriptBindingCapture_Write(c,s,in,d);if(!n && s==NativeScriptBinding_EffectsActive && in->type==NativeScriptType_Bool && nativeScriptBindings_SourceAlive(c)){((Fighter*)((HSD_GObj*)c->source)->user_data)->x2219_b0=in->as.u64!=0;return true;}if(n||s!=NativeScriptBinding_PositionX||in->type!=NativeScriptType_F32||!nativeScriptBindings_SourceAlive(c))return false;')
 b=b.replace(body(b,'Fighter_ChangeMotionState'),'').replace('#include "boundary.h"','#include "boundary.h"\n#include <melee/ft/native_script_binding_capture.h>\n#include <melee/ft/native_script_binding_common_properties.h>')
 b=b.replace('if(n||s!=NativeScriptBinding_PositionX||!nativeScriptBindings_SourceAlive(c))return false;',
  'if(!n&&s==NativeScriptBinding_ArticlesAttachedVisible)return nativeScriptBindingCommonProperties_Read(c,s,out,d);if(n||s!=NativeScriptBinding_PositionX||!nativeScriptBindings_SourceAlive(c))return false;')
 b=b.replace('if(n||s!=NativeScriptBinding_PositionX||in->type!=NativeScriptType_F32||!nativeScriptBindings_SourceAlive(c))return false;',
  'if(!n&&s==NativeScriptBinding_ArticlesAttachedVisible)return nativeScriptBindingCommonProperties_Write(c,s,in,d);if(n||s!=NativeScriptBinding_PositionX||in->type!=NativeScriptType_F32||!nativeScriptBindings_SourceAlive(c))return false;')
 b=b.replace(body(b,'ftCo_800BFFD0'),'')
 b=b.replace(body(b,'lb_8000B1CC'),'')
 b=b.replace(body(b,'HSD_JObjRemoveAll'),'')
 b=b.replace('if(n||s!=NativeScriptBinding_PositionX||!nativeScriptBindings_SourceAlive(c))return false;',
  'if(!n&&(s==NativeScriptBinding_FacingAnimationDirection||s==NativeScriptBinding_WalkType||(s>=NativeScriptBinding_ControlsThrowHorizontalThreshold&&s<=NativeScriptBinding_ControlsThrowDownThreshold)))return nativeScriptBindingFighterObservations_Read(c,s,out,d);if(n||s!=NativeScriptBinding_PositionX||!nativeScriptBindings_SourceAlive(c))return false;')
 b=b.replace('#include <melee/ft/native_script_binding_common_properties.h>', '#include <melee/ft/native_script_binding_common_properties.h>\n#include <melee/ft/native_script_binding_fighter_observations.h>')
 b=b.replace('if(n||s!=NativeScriptBinding_PositionX||!nativeScriptBindings_SourceAlive(c))return false;',
  'if(!n&&(s==NativeScriptBinding_DamageReactionSuppressed||s==NativeScriptBinding_DamageKnockbackLevel))return nativeScriptBindingCombat_Read(c,s,out,d);if(n||s!=NativeScriptBinding_PositionX||!nativeScriptBindings_SourceAlive(c))return false;')
 b=b.replace('if(n||s!=NativeScriptBinding_PositionX||!nativeScriptBindings_SourceAlive(c))return false;',
  'if(!n&&s==NativeScriptBinding_AirDriftThreshold)return nativeScriptBindingCommonProperties_Read(c,s,out,d);if(n||s!=NativeScriptBinding_PositionX||!nativeScriptBindings_SourceAlive(c))return false;')
 b=b.replace('if(n||s!=NativeScriptBinding_PositionX||in->type!=NativeScriptType_F32||!nativeScriptBindings_SourceAlive(c))return false;',
  'if(!n&&s==NativeScriptBinding_DamageForceTopBlastKO)return nativeScriptBindingCommonProperties_Write(c,s,in,d);if(n||s!=NativeScriptBinding_PositionX||in->type!=NativeScriptType_F32||!nativeScriptBindings_SourceAlive(c))return false;')
 b=b.replace('++presentation_callback_writes;', '++presentation_callback_writes;capture_hook_observe(fp);').replace('extern NativeSourcePackage capture_package;', 'extern NativeSourcePackage capture_package;\nvoid capture_hook_observe(Fighter*);')
 b=b.replace('if(!n&&s==NativeScriptBinding_ArticlesAttachedVisible)return nativeScriptBindingCommonProperties_Read',
  'if(!n&&(s==NativeScriptBinding_ArticlesAttachedVisible||s==NativeScriptBinding_ModelInstanceScaleY))return nativeScriptBindingCommonProperties_Read')
 boundary.write_text(b)
 jobj=ROOT/'src/sysdolphin/baselib/jobj.c';extra=out/'joint_leaves.c';extra.write_text('#include <melee/ft/ftcharacter_assets.h>\n#include <sysdolphin/baselib/jobj.h>\n#include <melee/ft/ft_0877.h>\n#include <melee/ft/ft_0881.h>\n#include <melee/ft/ftanim.h>\n#include <melee/it/it_26B1.h>\n#include <melee/lb/lb_00B0.h>\n#include <stdlib.h>\n'+'\n'.join(body(jobj.read_text(),fn) for fn in ['HSD_JObjGetFlags','HSD_JObjClearFlags','HSD_JObjSetMtxDirtySub'])+'\n'+'''void ft_PlaySFX(Fighter*f,enum_t s,u8 v,u8 p){(void)f;(void)s;(void)v;(void)p;abort();}
void ft_800881D8(Fighter*f,int s,u8 v,u8 p){(void)f;(void)s;(void)v;(void)p;abort();}
void ft_80088478(Fighter*f,int s,u8 v,u8 p){(void)f;(void)s;(void)v;(void)p;abort();}
void ft_80088510(Fighter*f,int s,u8 v,u8 p){(void)f;(void)s;(void)v;(void)p;abort();}
void ft_800885A8(Fighter*f,int s,u8 v,u8 p){(void)f;(void)s;(void)v;(void)p;abort();}
void ft_80088640(Fighter*f,int s,u8 v,u8 p){(void)f;(void)s;(void)v;(void)p;abort();}
void ft_80088328(Fighter*f,int s,u8 v,u8 p){(void)f;(void)s;(void)v;(void)p;abort();}
bool itIsHeavy(Item_GObj*g){(void)g;abort();}
Item_HoldKinds itGetHoldKind(Item_GObj*g){(void)g;abort();}
void ftAnim_800704F0(Fighter_GObj*g,int n,float v){(void)g;(void)n;(void)v;abort();}
void HSD_MObjReqAnim(HSD_MObj*m,float f){(void)m;(void)f;abort();}
void HSD_MObjAnim(HSD_MObj*m){(void)m;abort();}

''')
 extra.write_text(extra.read_text().replace(body(extra.read_text(),'ft_PlaySFX'),''))
 aobj=ROOT/'src/sysdolphin/baselib/aobj.c';aobj_text=aobj.read_text()
 foreach=aobj_text[aobj_text.index('static void callbackForeachFunc('):aobj_text.index('void HSD_AObjSetRate(')]
 (out/'accessory_clock.c').write_text('#include <sysdolphin/baselib/dat_import.h>\n#include <sysdolphin/baselib/fog.h>\n#include <stdarg.h>\n'+foreach+'\n'+body(aobj_text,'HSD_AObjSetRate')+'\n'+body(aobj_text,'HSD_AObjGetFlags'))
 dat_import=ROOT/'src/sysdolphin/baselib/dat_import.c'
 with (out/'accessory_clock.c').open('a') as f:f.write('\n#include <string.h>\n'+body(dat_import.read_text(),'HSD_DatImportAlloc'))
 hashes[str(dat_import)]=hashlib.sha256(dat_import.read_bytes()).hexdigest()
 accessory_source=ROOT/'tests/native/samus_grapple_constructor/fighter.constructor.fms'
 helper=accessory_source.read_text();helper=helper[helper.index('func Tether_CreateThrowAccessory('):]
 (out/'accessory_script.inc').write_text(json.dumps(helper)+'\n')
 hashes[str(accessory_source)]=hashlib.sha256(accessory_source.read_bytes()).hexdigest()
 hashes[str(out/'accessory_script.inc')]=hashlib.sha256((out/'accessory_script.inc').read_bytes()).hexdigest()
 samus=ROOT/'src/melee/ft/kinds/ftSamus/ftsamus.c';samus_text=samus.read_text()
 reference=body(samus_text,'ftSs_Init_80128770')+'\n'+body(samus_text,'ftSs_Init_CreateThrowGrappleBeam')
 reference=reference.replace('ftSs_Init_80128770','reference_nonlooping_rate').replace('ftSs_Init_CreateThrowGrappleBeam','reference_accessory_constructor')
 (out/'accessory_reference.inc').write_text(reference)
 hashes.update({str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in [aobj,samus,out/'accessory_clock.c',out/'accessory_reference.inc',HERE/'accessory_leaves.h',HERE/'accessory_cases.h']})
 names=['source','expression','statement','program','bytecode','vm','value','math','constants','bindings', 'binding_item_resources_catalog', 'binding_spawn_catalog','compile', 'binding_item_resources_resolve','metadata','item_program', 'resource_metadata','fighter_program','fighter_actor','fighter_hooks','effect_program','moves','move_lowering','events','event_payload','storage','execution','runtime','receiver','binding_receiver','actor_access','pose_metadata','hitbox_metadata','binding_spawn_resolve','binding_effect_resolve','binding_signal_resolve','binding_article_reference_resolve','binding_item_model_resolve','model_metadata','binding_setup','binding_presentation']
 units=[ROOT/'src/melee/ft'/('native_script_'+n+'.c') for n in names]+[ROOT/'src/melee/ft'/n for n in ['native_callback_lifetime.c','native_fighter_pose_path.c','native_fighter_pool.c','ftparts_table.c','ftparts_rotation.c','native_source_declarations.c','native_source_asset_validation.c','native_fighter_sound_policy.c','ftcharacter_asset_layout.c']]+[ROOT/'src/sysdolphin/baselib/dat.c']
 units += [ROOT/'src/melee/gm'/(n+'.c') for n in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(n+'.c') for n in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage']]
 extra_text=extra.read_text()
 for fn in ['ft_800881D8']:
  definition=[line for line in extra_text.splitlines(keepends=True) if line.startswith('void '+fn+'(')]
  assert len(definition)==1 and definition[0].rstrip().endswith('abort();}')
  extra_text=extra_text.replace(definition[0],'')
 extra.write_text(extra_text)
 units += sorted(f for f in (ROOT/'tests/native').glob('script_bindings_unavailable_*.c') if f.name not in ('script_bindings_unavailable_presentation.c','script_bindings_unavailable_capture.c','script_bindings_unavailable_visuals.c','script_bindings_unavailable_feedback.c','script_bindings_unavailable_fighter_observations.c','script_bindings_unavailable_common_properties.c','script_bindings_unavailable_common_movement.c','script_bindings_unavailable_jump.c','script_bindings_unavailable_air_physics.c','script_bindings_unavailable_attachments.c'))+[ROOT/'tests/native/script_receiver_unavailable_identity.c',HERE/'test.c',HERE/'unselected.c',boundary,unavailable,extra]
 units += [out/'accessory_clock.c', ROOT/'src/melee/ft/ftaccessory_assets.c']
 units += [ROOT/'src/melee/ft/native_script_binding_common_properties.c',ROOT/'src/melee/ft/native_script_binding_attachments.c',ROOT/'src/melee/ft/native_fighter_attachment.c']
 units += [ROOT/'src/melee/ft/native_script_binding_state.c']
 units += [ROOT/'src/melee/ft/native_script_binding_common_movement.c']
 units += [ROOT/'src/melee/ft/native_script_binding_jump.c',ROOT/'src/melee/ft/native_fighter_jump.c',ROOT/'src/melee/ft/native_script_binding_air_physics.c']
 units += [ROOT/'src/melee/ft/native_script_binding_fighter_observations.c']
 units += [ROOT/'src/melee/ft'/n for n in ['native_script_binding_capture.c','native_capture.c','native_fighter_capture.c','native_fighter_capture_rig.c','native_script_binding_visuals.c','native_script_binding_combat.c','native_fighter_pose.c','ftanim_tree.c']]
 native_paths={'src/melee/ft/fighter.c':['Fighter_UnkProcessGrab_8006CA5C'],'src/melee/ft/ftcommon.c':['ftCommon_8007E2D0','ftCommon_8007E2F4','ftCommon_8007F578','ftCommon_8007F5CC','ftCommon_InitGrab','ftCommon_8007EFC0'],'src/melee/ft/native_script_binding_properties.c':['nativeScriptBindings_SourceFighterLayout'],'src/melee/ft/kinds/ftCommon/ftCo_CaptureCaptain.c':['ftCo_8009CA0C'],'src/melee/ft/ftswing.c':['fn_800CCEC4','get_anim_id','ftCo_Attack_800CCF58'],'src/melee/ft/ftanim.c':['ftAnim_80070904','ftAnim_ApplyPartAnim','ftAnim_80070FB4','ftAnim_80070C48','some_inline','ftAnim_80070CC4','ftAnim_8006F994'],'src/melee/lb/lb_00B0.c':['lb_8000BECC','lb_8000BFF0']}
 native=[]
 walk_functions=['ftWalkCommon_GetWalkType','ftWalkCommon_GetWalkType_800DFBF8_fake','ftWalkCommon_800DFCA4','ftWalkCommon_800DFDDC','getWalkAccel','ftWalkCommon_800E0060']
 native_paths['src/melee/ft/ftwalkcommon.c']=walk_functions
 native_paths['src/melee/ft/ft_084E.c']=['ft_800850B4','ft_800850E0']
 movement_functions=['ftCommon_CalcGroundAccel_Deaccel','ftCommon_CalcGroundAccel_DashRun','ftCommon_SetSelfMovementFromGroundedMovement']
 native_paths['src/melee/ft/ftmovement.c']=movement_functions
 input_paths={'src/melee/ft/ftwalkcommon.c':['ftWalkCommon_800DFC70'],
  'src/melee/ft/ft_08A1.c':['ft_8008A1FC'],
  'src/melee/ft/kinds/ftCommon/ftCo_Turn.c':['ftCo_800C97A8'],
  'src/melee/mp/mpcoll.c':['mpColl_IsOnPlatform'],
  'src/melee/ft/kinds/ftCommon/ftCo_Pass.c':['ftCo_80099F1C'],
  'src/melee/ft/kinds/ftCommon/ftCo_Jump.c':['ftCo_Jump_GetInput'],
  'src/melee/ft/native_fighter_runtime.c':['nativeFighterRuntime_AnimationFamily']}
 input_reference=[]
 for relative,functions in input_paths.items():
  native_paths[relative]=native_paths.get(relative,[])+functions
  previous=subprocess.run(['git','show','6a69c522d:'+relative],cwd=ROOT,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
  input_reference.extend(body(previous,fn) for fn in functions)
 input_reference='\n'.join(input_reference)
 import re
 for fn in sorted([f for fs in input_paths.values() for f in fs],key=len,reverse=True):
  input_reference=re.sub(r'\b'+re.escape(fn)+r'\b','reference_'+fn,input_reference)
 (out/'input_reference.inc').write_text(input_reference+'\n')
 (out/'input_reference.json').write_text(json.dumps({'commit':'6a69c522d','body_sha256':hashlib.sha256(input_reference.encode()).hexdigest(),'alterations':['entry and internal helper names renamed only']},indent=2)+'\n')
 jump_paths={'src/melee/ft/kinds/ftCommon/ftCo_KneeBend.c':['ftCo_KneeBend_Check_ShortHop'],
  'src/melee/ft/kinds/ftCommon/ftCo_Jump.c':['ftCo_800CB110','ftCo_Jump_Phys_Inner'],
  'src/melee/mp/mpcoll.c':['mpUpdateFloorSkip'],
  'src/melee/ft/kinds/ftCommon/ftCo_Pass.c':['ftCo_8009A184']}
 jump_reference=[]
 for relative,functions in jump_paths.items():
  native_paths[relative]=native_paths.get(relative,[])+functions
  previous=subprocess.run(['git','show','95d8528b3:'+relative],cwd=ROOT,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
  jump_reference.extend(body(previous,fn) for fn in functions)
 native_paths['src/melee/ft/kinds/ftCommon/ftCo_KneeBend.c'][:0]=['check_short_hop']
 native_paths['src/melee/ft/kinds/ftCommon/ftCo_KneeBend.c'].append('ftCo_KneeBend_CheckShortHopAtTakeoff')
 jump_reference='\n'.join(jump_reference)
 for fn in sorted([f for fs in jump_paths.values() for f in fs],key=len,reverse=True):
  jump_reference=re.sub(r'\b'+re.escape(fn)+r'\b','reference_'+fn,jump_reference)
 (out/'jump_reference.inc').write_text(jump_reference+'\n')
 (out/'jump_reference.json').write_text(json.dumps({'commit':'95d8528b3','body_sha256':hashlib.sha256(jump_reference.encode()).hexdigest(),'alterations':['entry and internal helper names renamed only']},indent=2)+'\n')
 attached_path='src/melee/ft/kinds/ftCommon/ftCo_CaptureKoopa.c'
 native_paths[attached_path]=['ftCo_InitializeAttachedCapture']
 attached_reference=body(subprocess.run(['git','show','fc4b737ec:'+attached_path],cwd=ROOT,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout,'ftCo_InitializeAttachedCapture').replace('ftCo_InitializeAttachedCapture','reference_attached_capture')
 (out/'attached_capture_reference.inc').write_text(attached_reference+'\n')
 (out/'attached_capture_reference.json').write_text(json.dumps({'commit':'fc4b737ec','body_sha256':hashlib.sha256(attached_reference.encode()).hexdigest(),'alterations':['entry name renamed only']},indent=2)+'\n')
 damage_tick_paths={
  'src/melee/ft/ftmovement.c':['ftCommon_Fall','ftCommon_CalcSelfAccel_Deaccel','ftCommon_CalcGroundAccel_Deaccel','ftCommon_SetSelfMovementFromGroundedMovement'],
  'src/melee/ft/ft_084E.c':['ft_80084EEC','ft_80084F3C'],
  'src/melee/ft/kinds/ftCommon/ftCo_HammerWait.c':['ftCo_800C5240','ftCo_800C554C'],
  'src/melee/ft/kinds/ftCommon/ftCo_Damage.c':['ftCo_8008F744','ftCo_Damage_Phys']}
 damage_tick_reference=[]
 for relative,functions in damage_tick_paths.items():
  native_paths[relative]=list(dict.fromkeys(native_paths.get(relative,[])+functions))
  previous=subprocess.run(['git','show','2be04ddb3:'+relative],cwd=ROOT,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
  damage_tick_reference.extend(body(previous,fn) for fn in functions)
 damage_tick_reference='\n'.join(damage_tick_reference)
 for fn in sorted([f for fs in damage_tick_paths.values() for f in fs],key=len,reverse=True):
  damage_tick_reference=re.sub(r'\b'+re.escape(fn)+r'\b','damage_tick_reference_'+fn,damage_tick_reference)
 (out/'damage_tick_reference.inc').write_text(damage_tick_reference+'\n')
 (out/'damage_tick_reference.json').write_text(json.dumps({'commit':'2be04ddb3','body_sha256':hashlib.sha256(damage_tick_reference.encode()).hexdigest(),'alterations':['entry and internal helper names renamed only']},indent=2)+'\n')
 walk_reference=[]
 for relative,functions in [('src/melee/ft/ftmovement.c',movement_functions),('src/melee/ft/ftwalkcommon.c',walk_functions),('src/melee/ft/ft_084E.c',['ft_800850E0','ft_800850B4'])]:
  previous=subprocess.run(['git','show','8010badfe:'+relative],cwd=ROOT,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
  walk_reference += [body(previous,fn) for fn in functions]
 walk_reference='\n'.join(walk_reference)
 # Rename longest identifiers first to preserve the longer inline helper name.
 import re
 for fn in sorted(walk_functions+['ft_800850B4','ft_800850E0']+movement_functions,key=len,reverse=True):
  walk_reference=re.sub(r'\b'+re.escape(fn)+r'\b','reference_'+fn,walk_reference)
 (out/'walk_reference.inc').write_text(walk_reference+'\n')
 (out/'walk_reference.json').write_text(json.dumps({'commit':'8010badfe','body_sha256':hashlib.sha256(walk_reference.encode()).hexdigest(),'alterations':['entry and internal helper names renamed only']},indent=2)+'\n')
 native_paths['src/melee/ft/kinds/ftCommon/ftCo_CaptureDamageKoopa.c']=['ftCo_EnterAttachedCaptureMotion']
 native_paths['src/melee/ft/kinds/ftCommon/ftCo_Shouldered.c']=['ftCo_8009C5A4','ftCo_8009C640']
 victim_reference=[]
 for relative,functions in [('src/melee/ft/kinds/ftCommon/ftCo_CaptureDamageKoopa.c',['ftCo_EnterAttachedCaptureMotion']),('src/melee/ft/kinds/ftCommon/ftCo_Shouldered.c',['ftCo_8009C5A4','ftCo_8009C640'])]:
  previous=subprocess.run(['git','show','5cf0c21d6:'+relative],cwd=ROOT,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
  victim_reference += [body(previous,fn) for fn in functions]
 victim_reference='\n'.join(victim_reference).replace('ftCo_EnterAttachedCaptureMotion','reference_victim_motion').replace('ftCo_8009C5A4','reference_hold_motion').replace('ftCo_8009C640','reference_initialize_hold')
 (out/'victim_motion_reference.inc').write_text(victim_reference+'\n')
 (out/'victim_motion_reference.json').write_text(json.dumps({'commit':'5cf0c21d6','body_sha256':hashlib.sha256(victim_reference.encode()).hexdigest(),'alterations':['function names and internal hold-motion call rename only']},indent=2)+'\n')
 native_paths['src/melee/ft/ftcoll.c']=['ftColl_8007B62C','ftColl_8007B128','ftColl_HurtboxInit']
 native_paths['src/melee/ft/kinds/ftCommon/ftCo_YoshiEgg.c']=['enclosure_shrink','enclosure_hurtbox','ftCo_CaptureEnclosure_Enter']
 native_paths['src/melee/ft/ftlib.c']=['ftLib_800868D4']
 native_paths['src/melee/ft/ftcommon_data.c']=['ftCommon_GetModelScale']
 native_paths.setdefault('src/melee/ft/ftcommon.c',[]).append('ftCommon_UnlockECB')
 native_paths['src/melee/ft/kinds/ftCommon/ftCo_Damage.c']=native_paths.get('src/melee/ft/kinds/ftCommon/ftCo_Damage.c',[])+['ftCo_ScaleBy154','ftCo_Damage_CalcKnockbackLevel','ftCo_8008E984','calcAngle','ftCo_Damage_CalcAngle','ftCo_Damage_CalcVel','ftCo_Damage_CheckAirMotion','ftCo_Damage_SetMv8FromKbThreshold','ftCo_8008DCE0','ftCo_8008E908']
 native_paths['src/melee/ft/ftcommon.c'].append('ftCommon_8007F824')
 native_paths['src/melee/ft/ftcommon.c'].append('ftCommon_8007D92C')
 native_paths['src/melee/ft/kinds/ftCommon/ftCo_DamageFall.c']=['ftCo_80090780']
 native_paths['src/melee/ft/kinds/ftCommon/ftCo_CaptureCut.c']=['ftCo_800DC920']
 native_paths['src/melee/ft/fighter.c'].append('Fighter_UnkSetFlag_8006CFBC')
 drop_path='src/melee/ft/kinds/ftCommon/ftCo_CaptureCut.c'
 drop_previous=subprocess.run(['git','show','c090df153:'+drop_path],cwd=ROOT,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
 drop_reference=body(drop_previous,'ftCo_800DC920').replace('ftCo_800DC920','reference_unlink_capture')
 (out/'drop_reference.inc').write_text(drop_reference+'\n')
 (out/'drop_reference.json').write_text(json.dumps({'commit':'c090df153','body_sha256':hashlib.sha256(drop_reference.encode()).hexdigest(),'alterations':['entry name renamed only']},indent=2)+'\n')
 finish_reference=[]
 for relative,fn in [('src/melee/ft/ftcommon.c','ftCommon_8007D92C'),('src/melee/ft/kinds/ftCommon/ftCo_DamageFall.c','ftCo_80090780')]:
  previous=subprocess.run(['git','show','f3acd8313:'+relative],cwd=ROOT,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
  finish_reference.append(body(previous,fn).replace(fn,'reference_'+fn))
 finish_reference='\n'.join(finish_reference)
 (out/'action_finish_reference.inc').write_text(finish_reference+'\n')
 (out/'action_finish_reference.json').write_text(json.dumps({'commit':'f3acd8313','body_sha256':hashlib.sha256(finish_reference.encode()).hexdigest(),'alterations':['entry names renamed only']},indent=2)+'\n')
 native_paths['src/melee/it/item.c']=['Item_8026AE60']
 damage_text=(ROOT/'src/melee/ft/kinds/ftCommon/ftCo_Damage.c').read_text()
 begin=damage_text.index('int ftCo_803C5520[2][4][3]');native.append(damage_text[begin:damage_text.index('};',begin)+2])
 original_damage=subprocess.run(['git','show','94abb5792:src/melee/ft/kinds/ftCommon/ftCo_Damage.c'],cwd=ROOT,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
 damage_reference='\n'.join(body(original_damage,fn) for fn in ['ftCo_8008DCE0','ftCo_8008E908'])
 damage_reference=damage_reference.replace('ftCo_8008DCE0','reference_damage_entry').replace('ftCo_8008E908','reference_damage_default')
 (out/'damage_reference.inc').write_text(damage_reference+'\n')
 (out/'damage_reference.json').write_text(json.dumps({'commit':'94abb5792','body_sha256':hashlib.sha256(damage_reference.encode()).hexdigest(),'alterations':['entry names and inter-function call rename only']},indent=2)+'\n')
 native_paths['src/melee/ft/kinds/ftCommon/ftCo_CaptureWait.c']=['ftCo_CaptureWait_Setup']
 native_paths['src/melee/ft/kinds/ftCommon/ftCo_CatchPull.c']=['fn_800D9CE8']
 native_paths['src/melee/ft/ftcoll.c']=native_paths.get('src/melee/ft/ftcoll.c',[])+['ftColl_8007B7A4']
 native_paths['src/melee/ft/kinds/ftCommon/ftCo_Thrown.c']=['ftCo_800DE3FC']
 native_paths['src/melee/ft/kinds/ftCommon/ftCo_Thrown.c'].append('ftCo_800DE920')
 native_paths['src/melee/ft/kinds/ftCommon/ftCo_ThrownKoopa.c']=['ftCo_EnterAttachedThrow']
 native_paths['src/melee/ft/kinds/ftCommon/ftCo_Throw.c']=['ftCo_800DD398','ftCo_800DD724_inline1','ftCo_800DD724_inline2','ftCo_800DD724','ftCo_ThrowF_Phys']
 throw_reference=[]
 for relative,functions in [('src/melee/ft/kinds/ftCommon/inlines.h',['ftCo_Thrown_Enter']),('src/melee/ft/kinds/ftCommon/ftCo_ThrownKoopa.c',['ftCo_EnterAttachedThrow']),('src/melee/ft/kinds/ftCommon/ftCo_Thrown.c',['ftCo_800DE3FC','ftCo_800DE7C0']),('src/melee/ft/kinds/ftCommon/ftCo_Throw.c',['ftCo_800DD724_inline1','ftCo_800DD724_inline2','ftCo_800DD724','ftCo_ThrowF_Phys'])]:
  previous=subprocess.run(['git','show','ac62f15ba:'+relative],cwd=ROOT,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
  throw_reference += [body(previous,fn) for fn in functions]
 throw_reference='\n'.join(throw_reference)
 for old,new in [('ftCo_Thrown_Enter','reference_attached_enter'),('ftCo_EnterAttachedThrow','reference_attached_throw'),('ftCo_800DE3FC','reference_throw_pose'),('ftCo_800DD724','reference_throw_tick'),('ftCo_ThrowF_Phys','reference_throw_motion')]:throw_reference=throw_reference.replace(old,new)
 # Keep tick release calls on the same controlled leaf as production. The
 # standalone finish comparison renames only its declaration, not callers.
 throw_reference=throw_reference.replace('void ftCo_800DE7C0(', 'void reference_release_finish(')
 (out/'throw_reference.inc').write_text(throw_reference+'\n')
 presentation_previous=subprocess.run(['git','show','55124e067:src/melee/ft/kinds/ftCommon/ftCo_Throw.c'],cwd=ROOT,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
 presentation_reference=body(presentation_previous,'ftCo_800DD398').replace('ftCo_800DD398','reference_throw_entry',1)
 (out/'throw_presentation_reference.inc').write_text(presentation_reference+'\n')
 (out/'throw_reference.json').write_text(json.dumps({'commit':'ac62f15ba','body_sha256':hashlib.sha256(throw_reference.encode()).hexdigest(),'alterations':['entry names and internal helper calls renamed only']},indent=2)+'\n')
 reference_path='src/melee/ft/native_capture.c'
 reference_text=subprocess.run(['git','show','d2ba54c30:'+reference_path],cwd=ROOT,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
 reference=body(reference_text,'nativeCapture_EncloseCaught').replace('nativeCapture_EncloseCaught','original_enclose_caught')
 (out/'caught_reference.inc').write_text(reference+'\n')
 (out/'caught_reference.json').write_text(json.dumps({'commit':'d2ba54c30','body_sha256':hashlib.sha256(reference.encode()).hexdigest(),'alterations':['entry function rename only']},indent=2)+'\n')
 metadata=ROOT/'src/melee/ft/ft_0892.c';metadata_text=metadata.read_text()
 hashes[str(metadata)]=hashlib.sha256(metadata.read_bytes()).hexdigest()
 native.append(metadata_text[metadata_text.index('typedef struct {'):metadata_text.index('} UnkStruct89768;')+len('} UnkStruct89768;')])
 native += [body(metadata_text,fn) for fn in ['decode_action_metadata','ft_800895E0','ft_80089768','ft_80089460','ft_80089824']]
 stale=ROOT/'src/melee/ft/ft_0881.c';hashes[str(stale)]=hashlib.sha256(stale.read_bytes()).hexdigest()
 native += [body(stale.read_text(),fn) for fn in ['inlineB0','inlineC0','ft_800892A0']]
 jab=ROOT/'src/melee/ft/kinds/ftCommon/ftCo_Attack1.c';hashes[str(jab)]=hashlib.sha256(jab.read_bytes()).hexdigest()
 native += [body(jab.read_text(),fn) for fn in ['doAttack12Normal','doAttack12','doAttack13']]
 guard=ROOT/'src/melee/ft/kinds/ftCommon/ftCo_Guard.c';hashes[str(guard)]=hashlib.sha256(guard.read_bytes()).hexdigest()
 native += [body(guard.read_text(),fn) for fn in ['ftCo_800923B4','ftCo_800939B4']]
 dynamics=ROOT/'src/melee/ft/ftdynamics.c';hashes[str(dynamics)]=hashlib.sha256(dynamics.read_bytes()).hexdigest()
 native += [body(dynamics.read_text(),fn) for fn in ['nativeDynamics_CaptureActor','ftCo_8009E614','ftCo_8009E7B4']]
 fighter_text=(ROOT/'src/melee/ft/fighter.c').read_text()
 start=fighter_text.index('            if (nativeScriptFighter_CallbackBound(gobj, NativeScriptFighterCallback_SecondaryDamage))')
 stop=fighter_text.index('            if (!fp->no_kb)',start)
 native.append('static void dispatch_secondary_damage(Fighter_GObj* gobj) {\nFighter* fp=gobj->user_data;uint32_t callback_generation=fp->x8_spawnNum;\n'+fighter_text[start:stop]+'\n++damage_continuations;\n}')
 timer_start=fighter_text.index('        if (fp->throw_invincibility_frames) {')
 timer_end=fighter_text.index('        if (fp->x221D_b6) {',timer_start)
 native.append('static void advance_throw_invincibility(Fighter* fp) {\n'+fighter_text[timer_start:timer_end].replace('ftCo_800C0200(', 'timer_color_reset_observer(')+'\n}')
 swing_text=(ROOT/'src/melee/ft/ftswing.c').read_text();start=swing_text.index('int lbl_803C6D70');native.append(swing_text[start:swing_text.index('};',start)+2])
 native_paths['src/melee/ft/ft_084E.c']=list(dict.fromkeys(native_paths.get('src/melee/ft/ft_084E.c',[])+['ft_80084E1C']))
 native_paths['src/melee/ft/ftmovement.c']=list(dict.fromkeys(native_paths.get('src/melee/ft/ftmovement.c',[])+['ftCommon_FallFast','ftCommon_CalcSelfAccel_AccelToVelClamped','ftCommon_CalcSelfAccel_AccelToVelClampedFrom']))
 native_paths['src/melee/ft/ftcommon.c']=list(dict.fromkeys(native_paths.get('src/melee/ft/ftcommon.c',[])+['ftCommon_CheckFallFast']))
 native_paths['src/melee/ft/ft_081B.c']=list(dict.fromkeys(native_paths.get('src/melee/ft/ft_081B.c',[])+['ft_8008239C']))
 native_paths['src/melee/ft/ftcollision_ground.c']=['ft_80082838']
 shield_path='src/melee/ft/kinds/ftCommon/ftCo_ShieldBreakFly.c'
 native_paths[shield_path]=['ftCo_80098B20']
 shield_original=body(subprocess.run(['git','show','b091887fa:'+shield_path],cwd=ROOT,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout,'ftCo_80098B20')
 shield_reference=shield_original.replace('ftCo_80098B20','reference_shield_break')
 (out/'shield_break_reference.inc').write_text(shield_reference+'\n')
 (out/'shield_break_reference.json').write_text(json.dumps({'commit':'b091887fa','path':shield_path,'function':'ftCo_80098B20','body_sha256':hashlib.sha256(shield_original.encode()).hexdigest(),'adaptation':'Entry name only; common engine leaves controlled'},indent=2)+'\n')
 hashes.update({str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in [HERE/'shield_break_cases.h',HERE/'shield_break_leaves.h',out/'shield_break_reference.inc',out/'shield_break_reference.json']})
 for relative,functions in native_paths.items():
  path=ROOT/relative;hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest();native += [body(path.read_text(),fn) for fn in functions]
 thrown=(ROOT/'src/melee/ft/kinds/ftCommon/ftCo_Thrown.c').read_text()
 native += [body(thrown,fn) for fn in ['calcKnockbackAngle','calcFacingDir']]
 # The main release leaf stays controlled; separately execute the actual
 # post-release body through the real damage initializer and controlled DI.
 native.append(body(thrown,'ftCo_800DE7C0').replace('ftCo_800DE7C0','actual_release_finish'))
 (out/'actual_capture.inc').write_text('\n'.join(native))
 units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c',ROOT/'src/melee/it/native_article_animation_origins.c',ROOT/'src/melee/ft/native_script_binding_item_animation_prepare.c',ROOT/'src/melee/ft/native_source_motion_policy.c']
 units += [ROOT/'tests/native/script_capture_items_absent.c']
 inputs=units+[HERE/'table_cases.h',HERE/'part_animation_cases.h',HERE/'run.py',HERE/'README.md',HERE/'capture_settings_cases.h',HERE/'enclosure_settings_cases.h',HERE/'victim_policy_cases.h',HERE/'enclosure_release_cases.h',HERE/'caught_policy_cases.h',HERE/'capture_hook_cases.h',HERE/'bone_status_cases.h',HERE/'cue_cases.h',ROOT/'docs/scripting/capture.md',out/'caught_reference.inc',out/'caught_reference.json',original,PRE/'unavailable.c',PRE/'boundary.h',PRE/'extract.py',jobj,ROOT/'src/melee/ft/ftcharacter_assets.c',ROOT/'examples/drmario_full.fms',a.compiler]+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'));sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest();hashes.update({str(f):sha(f) for f in inputs});runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 hashes[str(HERE/'reaction_cases.h')]=sha(HERE/'reaction_cases.h')
 hashes.update({str(f):sha(f) for f in [HERE/'victim_motion_cases.h',out/'victim_motion_reference.inc',out/'victim_motion_reference.json']})
 hashes.update({str(f):sha(f) for f in [HERE/'throw_cases.h',HERE/'throw_invincibility_cases.h',HERE/'throw_presentation_cases.h',out/'throw_reference.inc',out/'throw_reference.json',out/'throw_presentation_reference.inc']})
 hashes.update({str(f):sha(f) for f in [HERE/'action_finish_cases.h',HERE/'action_finish_leaves.h',out/'action_finish_reference.inc',out/'action_finish_reference.json',ROOT/'tools/native_fighter_script_primitives.c']})
 hashes.update({str(f):sha(f) for f in [HERE/'drop_cases.h',HERE/'drop_leaves.h',out/'drop_reference.inc',out/'drop_reference.json']})
 hashes.update({str(f):sha(f) for f in [HERE/'walk_cases.h',HERE/'walk_leaves.h',out/'walk_reference.inc',out/'walk_reference.json',ROOT/'docs/scripting/movement.md']})
 hashes.update({str(f):sha(f) for f in [HERE/'input_query_cases.h',HERE/'input_query_leaves.h',out/'input_reference.inc',out/'input_reference.json',ROOT/'docs/scripting/fighter.md']})
 hashes.update({str(f):sha(f) for f in [HERE/'multi_jump_cases.h',HERE/'multi_jump_leaves.h',HERE/'attachment_cases.h',HERE/'attachment_leaves.h']})
 hashes.update({str(f):sha(f) for f in [HERE/'jump_cases.h',HERE/'jump_leaves.h',out/'jump_reference.inc',out/'jump_reference.json']})
 hashes.update({str(f):sha(f) for f in [HERE/'attached_capture_cases.h',out/'attached_capture_reference.inc',out/'attached_capture_reference.json']})
 hashes.update({str(f):sha(f) for f in [HERE/'damage_tick_cases.h',HERE/'damage_tick_leaves.h',out/'damage_tick_reference.inc',out/'damage_tick_reference.json']})
 hashes.update({str(f):sha(f) for f in [HERE/'damage_entry_cases.h',HERE/'damage_entry_leaves.h',out/'damage_reference.inc',out/'damage_reference.json']})
 common=['-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(PRE),'-I'+str(out)];commands=[]
 for stage,cmd in [('syntax',[str(a.compiler),*common,'-Wall','-Wextra','-Werror','-Wno-array-parameter','-fsyntax-only',str(ROOT/'src/melee/ft/native_script_binding_capture.c')]),('runtime',[str(a.compiler),*common,'-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]),('compile',[str(a.compiler),*common,'-ffunction-sections','-fdata-sections','-fno-zero-initialized-in-bss','-fuse-ld=lld','-Wl,/opt:ref','-Wl,/errorlimit:0',*map(str,units),str(out/'runtime.obj'),'-o',str(out/'test.exe')]),('fixture',[str(out/'test.exe')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode})
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-2500:])
 if any(sha(Path(f))!=h for f,h in hashes.items()):raise RuntimeError('input changed')
 receipt={'status':'PASS','result':json.loads(r.stdout),'input_sha256':hashes,'commands':commands,'caught_reference':json.loads((out/'caught_reference.json').read_text()),'scope':['actual catch-pull frame remapping and throw-victim setup schedulers, typed owned hooks, missing-hook fallback and source/peer retirement guards','interpreted caught enclosure/constraint and holder setup hook; native scale and capsule initializer; six pinned recipe comparisons','real source compiler, prepared hook ABI, actor/VM, lifetime tokens, physical Fighter/GObj pools','actual grab scheduler, capture bindings and owned source layout guard','actual contact receiver capture, identity and retirement checks','production canonical metadata decode, query dispatch and statistics reset versus native Luigi classification','production held-swing selector/dispatch with absent hook, motion-table and retirement guards','production partial-animation binding, native traversal and blend setup versus direct native call','script-owned deferred accessory helper and real effect-hitlag callback installation','production effects.spawn compiler/VM/top-level routing to explicitly unavailable feedback leaf','actual shield-entry dispatch with controlled common initialization','both actual dynamics-wind schedulers versus stock policies with controlled wind and bone leaves','exact native secondary-damage dispatch branch, callback ownership/reset and actor snapshot roundtrip'],'limits':['collision detection, animation/rig initialization, damage, audio and item interaction leaves controlled; no rendered match acceptance','metadata counter allocator and held-item classification predicate controlled','JObj animation evaluation, effect-active assignment and effect callback leaves controlled','additional unselected rebound/statistics/shake bindings abort if reached; owned package assets controlled; no DAT import or full Luigi/Falcon/Ganon gameplay admission','full hit scheduler, knockback/reaction path, shield rendering and dynamics evaluation outside this fixture','native Fighter and peer state held stable during actor-only snapshot restoration']}
 receipt['walk_reference']=json.loads((out/'walk_reference.json').read_text())
 receipt['input_reference']=json.loads((out/'input_reference.json').read_text())
 receipt['jump_reference']=json.loads((out/'jump_reference.json').read_text())
 receipt['attached_capture_reference']=json.loads((out/'attached_capture_reference.json').read_text())
 receipt['damage_tick_reference']=json.loads((out/'damage_tick_reference.json').read_text())
 receipt['shield_break_reference']=json.loads((out/'shield_break_reference.json').read_text())
 receipt['scope'].append('common shield-break entry and owned shield_break dispatch versus pinned native body; normal/top-blast policies, absent hook, physical actor and published program retirement at six engine boundaries')
 receipt['limits'].append('shield-break common reset, motion/animation setup, effect, sound and hurt-status leaves controlled; actual callback scheduling and launch assignments execute; no rendered shield break or complete Jigglypuff gameplay acceptance')
 receipt['scope'].append('damage/hitstun timer and damage-physics branch selection versus pinned native bodies; actual airborne hitstun gravity/friction, ground friction, hammer detection/recovery and owned program retirement')
 receipt['limits'].append('damage timer knockback-exit callback and normal airborne physics controlled; no full knockback/collision simulation or DK cargo gameplay acceptance')
 receipt['scope'].append('attached-capture initialization versus pinned native body, through owned VM and direct binding; owner/victim freeze distinction and actor/program retirement at eight initialization boundaries')
 receipt['limits'].append('attached-capture cleanup, bone attachment, motion entry, animation evaluation and ground-to-air clearing are controlled; no complete Bowser or Mewtwo match acceptance')
 receipt['scope'].append('costume attachment root lookup, selected skin and fallback costume, actual native load/dynamics/cleanup through source compiler, VM, actor Load scope and binding; malformed text, visibility spans and source retirement rejection')
 receipt['limits'].append('attachment DAT root lookup executes; graph import, JObj creation and draw-list/material/visibility initialization, cloth-chain allocation/copy/free and rendering are controlled leaves; not rendered Jigglypuff costume or cloth acceptance')
 receipt['scope'].append('shared short-hop detection, jump launch, first-frame jump physics and platform-drop entry versus pinned native bodies; typed startup preparation and retirement after platform motion change')
 receipt['limits'].append('jump audio, subsequent air physics, ground-to-air clearing and drift clamp controlled; no rendered jump, real platform collision or complete cargo activation')
 receipt['scope'].append('ordinary walk/stop/turn/platform-drop/jump input predicates and victim animation-family query versus pinned native bodies; exact common throw threshold Float observations')
 receipt['limits'].append('input-query cohort controls platform line flags; native platform classifier and input predicates execute; complete fighter activation remains open')
 receipt['scope'].append('shared walk entry, velocity classification, animation rate, walk physics and animation-facing ground physics; 983 pinned native/interpreter comparisons plus retirement and argument rejection cases')
 receipt['limits'].append('walk floor material multipliers, motion entry and animation evaluation controlled; actual native acceleration/deceleration and slope projection bodies execute; no complete DK or Bowser gameplay admission')
 receipt['scope'].append('exact authored Samus throw accessory helper through compiler/actor/VM versus the current native constructor; 144 scale/rate/loop cases, six retirement cases, eight loop-policy cases and 12 invalid runtime cases; actual HSD animation traversal and rate setters')
 receipt['limits'].append('accessory descriptors and runtime graph construction/loading/evaluation/follow are controlled in this VM cohort; real retail recipe import is covered separately by native_fighter_character_assets; no rendered accessory or complete Samus acceptance')
 receipt['scope'].append('actual common throw entry, invincibility initializer and counter tick slice dispatch typed presentation after complete victim setup and before common throw invincibility; all four throws, stock/owned victims, rates, absent hook and callback retirement')
 receipt['limits'].append('motion entry, animation, invincibility overlay query/clear and stock Samus accessory construction are controlled observer leaves; extracted timer slice redirects only the overlay-clear call to an observer; no full fighter tick or rendered throw accessory acceptance')
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['result']))
if __name__=='__main__':main()
