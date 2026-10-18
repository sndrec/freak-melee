"""Actual bone queries through compiler/actor/VM and native matrix kernels."""
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
 original=PRE/'boundary.c';boundary=out/'boundary.c';text=original.read_text();text=text.replace(body(text,'nativeFighterRuntime_SourcePackage'),'').replace(body(text,'ftParts_GetBoneIndex'),'').replace(body(text,'lb_8000B1CC'),'');text=text.replace(r'#include <stdlib.h>\n#include <stdio.h>\n#include <stdarg.h>','#include <stdlib.h>\n#include <stdio.h>\n#include <stdarg.h>');text=text.replace(body(text,'nativeScriptBindings_Read'),'').replace(body(text,'nativeScriptBindings_Write'),'').replace('#include "actual_context.h"','')
 boundary.write_text(text)
 unavailable=out/'unavailable.c';unavailable.write_text((PRE/'unavailable.c').read_text())
 missing=unavailable.read_text()
 for name in ['nativeScriptItem_HasActors','nativeScriptItem_ReferencesProgram','nativeScriptItem_View','nativeScriptItem_Spec','nativeScriptItem_Status','nativeScriptItem_Creator','nativeScriptItem_AttachmentOwner','nativeScriptItem_CanBeginScene','nativeScriptItem_BeginScene','nativeScriptItem_RetireScene']:
  missing=missing.replace(body(missing,name),'')
 missing += '\n#define REJECT_READ(name) bool name(NativeScriptRuntimeContext*c,uint32_t s,NativeScriptValue*v,NativeScriptDiagnostic*d){(void)c;(void)s;(void)v;(void)d;return false;}\n#define REJECT_WRITE(name) bool name(NativeScriptRuntimeContext*c,uint32_t s,const NativeScriptValue*v,NativeScriptDiagnostic*d){(void)c;(void)s;(void)v;(void)d;return false;}\nREJECT_READ(nativeScriptBindingKinetics_Read) REJECT_READ(nativeScriptBindingState_Read) REJECT_READ(nativeScriptBindingCombat_Read)\nREJECT_WRITE(nativeScriptBindingKinetics_Write) REJECT_WRITE(nativeScriptBindingCombat_Write)\nbool nativeScriptBindingSetup_Read(NativeScriptRuntimeContext*c,uint32_t s,const NativeScriptValue*a,uint32_t n,NativeScriptValue*v,NativeScriptDiagnostic*d){(void)c;(void)s;(void)a;(void)n;(void)v;(void)d;return false;}\nbool nativeScriptBindingSetup_Write(NativeScriptRuntimeContext*c,uint32_t s,const NativeScriptValue*a,uint32_t n,const NativeScriptValue*v,NativeScriptDiagnostic*d){(void)c;(void)s;(void)a;(void)n;(void)v;(void)d;return false;}\n'
 unavailable.write_text(missing)
 jobj=ROOT/'src/sysdolphin/baselib/jobj.c';cls=ROOT/'src/sysdolphin/baselib/class.c'
 native=[]
 for relative,names in [
  ('src/sysdolphin/baselib/jobj.c',['has_scl','HSD_JObjGetFlags','HSD_JObjClearFlags','HSD_JObjSetMtxDirtySub','HSD_JObjMakeMatrix','HSD_JObjSetupMatrixSub']),
  ('src/sysdolphin/baselib/class.c',['hsdObjIsDescendantOf']),
  ('src/melee/lb/lbjoint_position.c',['lb_8000B1CC']),
  ('src/sysdolphin/baselib/mtx.c',['HSD_MtxSRT','HSD_MtxSRTQuat']),
  ('extern/dolphin/src/dolphin/mtx/mtx.c',['C_MTXCopy','C_MTXConcat','C_MTXQuat','C_MTXScale']),
  ('extern/dolphin/src/dolphin/mtx/mtxvec.c',['C_MTXMultVec']),
  ('extern/dolphin/src/dolphin/mtx/vec.c',['C_VECAdd','C_VECScale','C_VECDotProduct'])]:
  path=ROOT/relative;current=path.read_text();hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
  reference=subprocess.run(['git','show','d25ac9faa:'+relative],cwd=ROOT,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
  for name in names:
   actual=body(current,name)
   if actual!=body(reference,name):raise RuntimeError('Native reference changed: '+name)
   native.append(actual)
 aurora=ROOT.parent/'aurora/lib/dolphin/mtx/mtx.c';hashes[str(aurora)]=hashlib.sha256(aurora.read_bytes()).hexdigest();native.append(body(aurora.read_text(),'C_MTXTrans'))
 extra=out/'native_bodies.c'
 extra.write_text('#include "native_leaves.h"\nstatic void resolveIKJoint1(HSD_JObj* j){(void)j;abort();}\nstatic void resolveIKJoint2(HSD_JObj* j){(void)j;abort();}\n'+'\n'.join(native)+'\n')
 names=['source','expression','statement','program','bytecode','vm','value','math','constants','bindings', 'binding_item_resources_catalog', 'binding_spawn_catalog','compile', 'binding_item_resources_resolve','metadata','item_program', 'resource_metadata','fighter_program','fighter_actor','fighter_hooks','effect_program','moves','move_lowering','events','event_payload','storage','execution','runtime','receiver','binding_receiver','actor_access','pose_metadata','binding_spawn_resolve','binding_effect_resolve','binding_signal_resolve','binding_article_reference_resolve','binding_item_model_resolve','model_metadata','binding_presentation','binding_properties']
 units=[ROOT/'src/melee/ft'/('native_script_'+n+'.c') for n in names]+[ROOT/'src/melee/ft'/n for n in ['native_callback_lifetime.c','ftparts_table.c','ftparts_rotation.c','native_fighter_pose_path.c','native_fighter_pool.c','native_source_declarations.c','native_source_asset_validation.c','native_fighter_sound_policy.c','ftcharacter_asset_layout.c','native_source_motion_policy.c','native_source_bank.c']]+[ROOT/'src/sysdolphin/baselib/dat.c']
 units += [ROOT/'src/melee/gm'/(n+'.c') for n in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(n+'.c') for n in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage']]
 units += sorted(f for f in (ROOT/'tests/native').glob('script_bindings_unavailable_*.c') if f.name not in ['script_bindings_unavailable_presentation.c','script_bindings_unavailable_item_birth.c'])+[ROOT/'tests/native/script_receiver_unavailable_identity.c',HERE/'test.c',boundary,unavailable,extra]
 units += [ROOT/'src/melee/it/native_script_item_actor.c',ROOT/'src/melee/it/native_script_item_snapshot.c',ROOT/'src/melee/ft/native_fighter_rig.c']
 units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c',ROOT/'src/melee/it/native_article_animation_origins.c',ROOT/'src/melee/ft/native_script_binding_item_animation_prepare.c',HERE/'leaves.c']
 units += [ROOT/'src/melee/ft/native_script_hitbox_metadata.c', HERE/'unselected_accessory.c', ROOT/'tests/native/script_graph_observers.c']
 units += [ROOT/'src/melee/gm'/(n+'.c') for n in ['native_match_arena','native_match_scene']]
 units += [ROOT/'src/melee/gr'/(n+'.c') for n in ['native_ground_pool','native_green_grid','native_green_grid_snapshot']]
 units += [ROOT/'src/melee/it'/(n+'.c') for n in ['native_item_pool','native_item_link_pool']]
 inputs=units+list(HERE.glob('*.h'))+[ROOT/'examples/fox_wip.fms',ROOT/'examples/falco_wip.fms',HERE/'run.py',HERE/'README.md',original,PRE/'unavailable.c',PRE/'boundary.h',PRE/'extract.py',jobj,cls,HERE/'native_leaves.h',a.compiler]+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'));sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest();hashes.update({str(f):sha(f) for f in inputs});runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 common=['-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(PRE),'-I'+str(HERE),'-I'+str(out)];commands=[]
 for stage,cmd in [('runtime',[str(a.compiler),*common,'-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]),('compile',[str(a.compiler),*common,'-ffunction-sections','-fdata-sections','-fno-zero-initialized-in-bss','-fuse-ld=lld','-Wl,/opt:ref','-Wl,/errorlimit:0',*map(str,units),str(out/'runtime.obj'),'-o',str(out/'test.exe')]),('fixture',[str(out/'test.exe')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode})
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-12000:])
 if any(sha(Path(f))!=h for f,h in hashes.items()):raise RuntimeError('input changed')
 receipt={'status':'PASS','result':json.loads(r.stdout),'native_reference':'d25ac9faa','input_sha256':hashes,'commands':commands,
  'scope':['actual compiler/Fighter actor/VM and top-level Presentation/property routing','raw joint translation and input-age storage, type/range/neighbor checks','actual native semantic mapping/world sampling/HSD-Dolphin kernels unchanged from d25ac9faa; actual Aurora C_MTXTrans pinned separately','actual fixed Fighter/GObj pools and owned source/storage guards'],
  'controlled':['source rig/DAT metadata and physical graph construction','unrelated native leaves and IK paths unavailable','RObj update and class initialization instrumentation for continuation hazards'],
  'limits':['no retail Link asset/package, renderer or game acceptance','no arbitrary IK/RObj animation parity; controlled RObj callback preserves static graph memory during physical source reuse','no new allocation; fixture preconstructs SRT scale storage']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['result']))
if __name__=='__main__':main()
