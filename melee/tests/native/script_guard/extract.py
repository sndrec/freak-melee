"""Extract exact native guard bodies and bounded ShieldHit continuation slices."""
import hashlib, importlib.util, json, re, subprocess
from pathlib import Path

def extract(root,out):
 root=Path(root);out=Path(out);hashes={}
 path=root/'tests/native/script_form_feedback/extract.py'
 spec=importlib.util.spec_from_file_location('guard_body',path)
 module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
 hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 def read(relative):
  p=root/relative;hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest();return p.read_text()
 properties=read('src/melee/ft/native_script_binding_properties.c')
 guards=[re.sub(r'\berror\b','guard_admission_error',module.body(properties,n)) for n in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout']]
 at=guards[1].index('{')+1;guards[1]=guards[1][:at]+'\n guard_capture_probe(context);'+guards[1][at:]
 (out/'source_guards.inc').write_text('\n'.join(guards)+'\n')
 common_path='src/melee/ft/kinds/ftCommon/ftCo_Guard.c'
 common_body=module.body(read(common_path),'ftCo_80092BCC')
 common_reference=subprocess.run(['git','show','829748389:'+common_path],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
 if common_body!=module.body(common_reference,'ftCo_80092BCC'):raise RuntimeError('Common guard release-input body changed')
 (out/'native_common_guard.inc').write_text(common_body+'\n')
 transition_names=['ftCo_800923B4','ftCo_800928CC','ftCo_80092BE8',
  'ftCo_80092E50','ftCo_80093850','ftCo_800939B4','ftCo_80092BE8_inline',
  'ftCo_800928CC_inline_arg','ftCo_GuardOn_Anim_inline']
 # Execute complete production scheduler bodies. Their unrelated stock
 # animation/effect paths remain explicit aborting leaves in this cohort.
 (out/'native_guard_transitions.inc').write_text('\n'.join(
  module.body(read(common_path),n) for n in transition_names)+'\n')
 common_source=read(common_path)
 operation_names=['ftCo_ShieldScale','ftCo_80091D58','update_shield_alpha',
  'ftCo_80092450_inline','ftCo_80092450','ftCo_8009370C','ftCo_800925A4',
  'ftCo_80093BC0','ftCo_80093240','ftCo_800932DC']
 (out/'native_shield_operations.inc').write_text('\n'.join(
  module.body(common_source,n) for n in operation_names)+'\n')
 flow_groups=[
  ('src/melee/ft/ftmovement.c',['ftCommon_CalcGroundAccel_Deaccel','ftCommon_SetSelfMovementFromGroundedMovement']),
  ('src/melee/ft/ft_084E.c',['ft_80084F3C']),
  ('src/melee/ft/ftcoll.c',['ftColl_8007AEF8']),
  ('src/melee/ft/ft_0DF1.c',['ftCo_800DF8B0','ftCo_800DF8E8']),
  ('src/melee/ft/kinds/ftCommon/ftCo_Escape.c',['inlineA1','inlineB0','ftCo_80099314','ftCo_800992A8','ftCo_800998EC','ftCo_80099894','ftCo_8009917C','ftCo_80099794','ftCo_8009980C']),
  ('src/melee/ft/kinds/ftCommon/ftCo_ItemThrow.c',['ftCo_8009515C']),
  ('src/melee/ft/kinds/ftCommon/ftCo_Pass.c',['ftCo_80099F1C','ftCo_8009A080']),
  (common_path,['ftCo_80093694','inlineC0','ftCo_GuardReflect_IASA','ftCo_Guard_Phys','ftCo_GuardSetOff_Phys','ftCo_Guard_Coll','ftCo_GuardSetOff_Coll'])]
 flow_bodies=[];flow_reference=[];flow_names=[];flow_commit='926b6e33d'
 for relative,names in flow_groups:
  current=read(relative)
  before=subprocess.run(['git','show',flow_commit+':'+relative],cwd=root,capture_output=True,
   text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
  flow_bodies.extend(module.body(current,n) for n in names)
  flow_reference.extend(module.body(before,n) for n in names);flow_names.extend(names)
 (out/'native_guard_flow.inc').write_text('\n'.join(flow_bodies)+'\n')
 reference_flow='\n'.join(flow_reference)
 for n in sorted(flow_names,key=len,reverse=True):
  reference_flow=re.sub(r'\b'+re.escape(n)+r'\b','flow_reference_'+n,reference_flow)
 # Entry semantics have their own connected scheduler fixture. The predicate
 # oracle's unowned stack Fighter uses explicit transition delivery leaves.
 for n in ['ftCo_80093850','ftCo_80092BE8']:
  reference_flow=re.sub(r'\b'+n+r'\b','flow_reference_'+n,reference_flow)
 (out/'native_guard_flow_reference.inc').write_text(reference_flow+'\n')
 (out/'guard_flow_reference.json').write_text(json.dumps({'commit':flow_commit,
  'bodies_sha256':hashlib.sha256('\n'.join(flow_reference).encode()).hexdigest(),
  'alteration':'renamed included definitions/calls; powershield/release entry use controlled delivery leaves'},indent=2)+'\n')
 jump_groups=[('src/melee/lb/lb_00B0.c',['lb_8000B074']),
  ('src/melee/ft/ftanim.c',['ftAnim_IsFramesRemaining']),
  ('src/melee/ft/kinds/ftCommon/ftCo_FallAerial.c',['ftCo_FallAerial_Enter']),
  ('src/melee/ft/kinds/ftCommon/ftCo_JumpAerial.c',['ftCo_JumpAerial_Anim'])]
 jump_bodies=[];jump_reference=[];jump_names=[];jump_commit='040bc5613'
 for relative,names in jump_groups:
  current=read(relative);before=subprocess.run(['git','show',jump_commit+':'+relative],cwd=root,
   capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
  jump_bodies.extend(module.body(current,n) for n in names)
  jump_reference.extend(module.body(before,n) for n in names);jump_names.extend(names)
 (out/'native_jump_policy.inc').write_text('\n'.join(jump_bodies)+'\n')
 jump_oracle='\n'.join(jump_reference)
 for n in sorted(jump_names,key=len,reverse=True):
  jump_oracle=re.sub(r'\b'+n+r'\b','jump_reference_'+n,jump_oracle)
 (out/'native_jump_policy_reference.inc').write_text(jump_oracle+'\n')
 (out/'jump_policy_reference.json').write_text(json.dumps({'commit':jump_commit,
  'bodies_sha256':hashlib.sha256('\n'.join(jump_reference).encode()).hexdigest(),
  'alteration':'renamed included definitions and their calls only'},indent=2)+'\n')
 reference_commit='7ff672421'
 operation_reference=subprocess.run(['git','show',reference_commit+':'+common_path],cwd=root,
  capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
 reference_names=['inlineB0','inlineD0','ftCo_80092450_inline','ftCo_8009370C',
  'ftCo_800925A4','ftCo_80093BC0','ftCo_80093240','ftCo_800932DC']
 reference_bodies=[module.body(operation_reference,n) for n in reference_names]
 reference_operations='\n'.join(reference_bodies)
 for n in sorted(reference_names,key=len,reverse=True):
  reference_operations=re.sub(r'\b'+re.escape(n)+r'\b','reference_'+n,reference_operations)
 reference_operations=reference_operations.replace('!gobj;','(void)gobj;')
 (out/'native_shield_operations_reference.inc').write_text(reference_operations+'\n')
 (out/'shield_operations_reference.json').write_text(json.dumps({
  'commit':reference_commit,'bodies_sha256':hashlib.sha256('\n'.join(reference_bodies).encode()).hexdigest(),
  'alteration':'only renamed symbols and replaced original effect-free !gobj expression with (void)gobj'},indent=2)+'\n')
 coll=read('src/melee/ft/ftcoll.c')
 reflector_reference=subprocess.run(['git','show',reference_commit+':src/melee/ft/ftcoll.c'],cwd=root,
  capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
 if module.body(coll,'ftColl_CreateReflectHit')!=module.body(reflector_reference,'ftColl_CreateReflectHit'):
  raise RuntimeError('Shared reflector constructor changed from shield operation reference')
 hurtbox_names=['ftColl_HurtboxInit','ftColl_8007B0C0']
 hurtbox_reference=subprocess.run(['git','show','7fde757c9:src/melee/ft/ftcoll.c'],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
 hurtbox_bodies=[module.body(coll,n) for n in hurtbox_names]
 if hurtbox_bodies!=[module.body(hurtbox_reference,n) for n in hurtbox_names]:raise RuntimeError('Shared hurtbox initializer/status bodies changed')
 (out/'native_hurtbox.inc').write_text('\n'.join(hurtbox_bodies)+'\n')
 (out/'native_hurtbox_reference.inc').write_text('\n'.join(module.body(hurtbox_reference,n).replace(n,'hurtbox_reference_init' if n=='ftColl_HurtboxInit' else 'hurtbox_reference_set_all') for n in hurtbox_names)+'\n')
 async_path='src/melee/ef/efasync.c'
 async_source=read(async_path)
 async_reference=subprocess.run(['git','show','7f6ab2bd6:'+async_path],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
 async_names=['efAsync_QueueProcessDeferred','efAsync_QueueFlush','efAsync_QueueClear','efAsync_Spawn']
 async_bodies=[module.body(async_source,n) for n in async_names]
 if async_bodies!=[module.body(async_reference,n) for n in async_names]:raise RuntimeError('Native effect queue bodies changed')
 (out/'native_async.inc').write_text('\n'.join(async_bodies)+'\n')
 (out/'native_class.inc').write_text(module.body(read('src/sysdolphin/baselib/class.c'),'hsdObjIsDescendantOf')+'\n')
 (out/'native_guard.inc').write_text('\n'.join(module.body(coll,n) for n in ['ftColl_8007B1B8','ftColl_8007AEE0','ftColl_CreateReflectHit'])+'\n')
 jobj=read('src/sysdolphin/baselib/jobj.c')
 (out/'native_joint_dirty.inc').write_text('\n'.join(module.body(jobj,n)
  for n in ['HSD_JObjGetFlags','HSD_JObjSetMtxDirtySub'])+'\n')
 fighter=read('src/melee/ft/fighter.c')
 start=fighter.index('} else if (fp->x19A4) {',fighter.index('void Fighter_ProcessHit_8006D1EC'))+len('} else ')
 end=fighter.index('} else if (fp->dmg.int_value)',start)
 branch=fighter[start:end]+'}\n'
 reset=re.findall(r'^\s*fp->x19A4 = 0;',fighter[fighter.index('void Fighter_ProcessHit_8006D1EC'):],re.M)
 if len(reset)!=1:raise RuntimeError('guard damage reset changed')
 (out/'native_guard_schedule.inc').write_text('static void guard_schedule(Fighter_GObj* gobj,bool bool3){\nFighter* fp=gobj->user_data;uint32_t callback_generation=fp->x8_spawnNum;float hitlag_damage=0;\n'+branch+'\n++guard_completed_tails;guard_hitlag_damage=hitlag_damage;\n'+reset[0]+'\n}\n')
 commit=subprocess.run(['git','rev-parse','HEAD'],cwd=root,capture_output=True,text=True,check=True).stdout.strip()
 before=subprocess.run(['git','show',commit+':src/melee/ft/ftcoll.c'],cwd=root,capture_output=True,text=True,check=True).stdout
 reference='\n'.join(module.body(before,n).replace(n,'guard_reference_create' if n=='ftColl_8007B1B8' else 'guard_reference_update') for n in ['ftColl_8007B1B8','ftColl_8007AEE0'])
 before_fighter=subprocess.run(['git','show',commit+':src/melee/ft/fighter.c'],cwd=root,capture_output=True,text=True,check=True).stdout
 a=before_fighter.index('} else if (fp->x19A4) {',before_fighter.index('void Fighter_ProcessHit_8006D1EC'))+len('} else ')
 b=before_fighter.index('} else if (fp->dmg.int_value)',a)
 reference+='\nstatic void guard_reference_schedule(Fighter_GObj* gobj,bool bool3){\nFighter* fp=gobj->user_data;uint32_t callback_generation=fp->x8_spawnNum;float hitlag_damage=0;\n'+before_fighter[a:b]+'}\n++reference_completed_tails;reference_hitlag_damage=hitlag_damage;fp->x19A4=0;\n}\n'
 (out/'native_guard_reference.inc').write_text(reference+'\n')
 (out/'reference_source.json').write_text(json.dumps({'commit':commit,'bodies_sha256':hashlib.sha256(reference.encode()).hexdigest()},indent=2)+'\n')
 return hashes
