"""Connected owned Item VM and unchanged native async/facing particle bodies."""
import argparse,hashlib,importlib.util,json,re,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];REFERENCE='4b32ba62a'
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh private A: output required')
 out.mkdir(parents=True)
 spec=importlib.util.spec_from_file_location('source_extract',HERE.parent/'script_form_feedback/extract.py');ex=importlib.util.module_from_spec(spec);spec.loader.exec_module(ex);body=ex.body
 hashes=ex.extract(ROOT,out/'abi');sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest()
 original=HERE.parent/'script_article_feedback/test.c';base=original.read_text();base=base[:base.index('int main(void)')];base=base.replace(body(base,'efSync_Spawn'),'').replace('roots[512]','roots[4096]').replace('allocated==512','allocated==4096')
 start=base.index(' const char* source=');end=base.index(' NativeScriptCompileRequest request=',start)
 source='fighter:\n    behavior independent\narticle Probe states 1:\n    state int effect\n    state int completed\n    state int scale_raw\n    on spawned:\n        item.spawn_async_joint_effect(item.state.effect, "", 0)\n        item.state.completed = item.state.completed + 1\n    on created:\n        item.spawn_async_joint_effect(item.state.effect, "0/1", 3)\n        item.state.completed = item.state.completed + 1\n    on accessory:\n        item.spawn_async_joint_effect(item.state.effect, "0", 3)\n        item.state.completed = item.state.completed + 1\n    on stage_landed:\n        item.spawn_async_joint_effect(item.state.effect, "0/1", 1)\n        item.state.completed = item.state.completed + 1\n    on stage_left:\n        item.spawn_async_joint_effect(item.state.effect, "", 4, float(item.state.scale_raw) * 0.25)\n        item.state.completed = item.state.completed + 1\nitem Snack damage 4 angle 45 lifetime 90:\n    state int effect\n    state int completed\n    on tick:\n        item.spawn_async_joint_effect(item.state.effect, "", 3)\n        item.state.completed = item.state.completed + 1\n'
 base=base[:start]+' const char* source='+json.dumps(source)+';\n'+base[end:]
 base=base.replace('NativeScriptItemHook_Created:NativeScriptItemHook_Accessory', 'NativeScriptItemHook_Created:!strcmp(e.descriptor->name,"stage_landed")?NativeScriptItemHook_StageLanded:!strcmp(e.descriptor->name,"stage_left")?NativeScriptItemHook_StageLeft:NativeScriptItemHook_Accessory')
 (out/'controlled_article.inc').write_text(base);hashes[str(original)]=sha(original)
 kernels=[]
 for relative,names in [('src/sysdolphin/baselib/class.c',['ClassInfoInit','_hsdClassInit','hsdNew','hsdObjIsDescendantOf']),('src/sysdolphin/baselib/jobj.c',['JObjInit','HSD_JObjAlloc']),('src/melee/ef/eflib.c',['efLib_DestroyAll','eflib_create_generator_add_appsrt','efLib_CreateGenerator_AppSRT_SetFacingDir']),('src/sysdolphin/baselib/generator.c',['hsd_8039EFAC']),('src/melee/ef/efasync.c',['efAsync_QueueProcessDeferred','efAsync_QueueFlush','efAsync_QueueClear','efAsync_Spawn','efAsync_QueueInit'])]:
  path=ROOT/relative;text=path.read_text();hashes[str(path)]=sha(path)
  ref=subprocess.run(['git','show',REFERENCE+':'+relative],cwd=ROOT,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
  for name in names:
   actual=body(text,name)
   if actual!=body(ref,name):raise RuntimeError('native body changed: '+name)
   kernels.append(actual)
 (out/'native_bodies.inc').write_text('\n\n'.join(kernels)+'\n')
 path=ROOT/'src/melee/ef/efsync.c';text=path.read_text();hashes[str(path)]=sha(path);ref=subprocess.run(['git','show',REFERENCE+':src/melee/ef/efsync.c'],cwd=ROOT,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
 cases=[]
 for identifier in [0x4E9,0x4EA,0x4EB,0x4F1,0x510]:
  def case(s):
   start=s.index('    case '+hex(identifier).upper().replace('0X','0x')+':');return s[start:s.index('    case ',start+10)]
  actual=case(text)
  if actual!=case(ref):raise RuntimeError('native case changed '+hex(identifier))
  cases.append(actual)
 (out/'native_sync_cases.inc').write_text('\n'.join(cases))
 path=ROOT/'src/melee/ef/efalt.c';text=path.read_text();hashes[str(path)]=sha(path);ref=subprocess.run(['git','show',REFERENCE+':src/melee/ef/efalt.c'],cwd=ROOT,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
 cases=[]
 for identifier in [0x48D,0x49A,0x4B0]:
  actual=case(text)
  if actual!=case(ref):raise RuntimeError('native alternate case changed '+hex(identifier))
  cases.append(actual)
 (out/'native_alt_cases.inc').write_text('\n'.join(cases))
 names=['source','expression','statement','program','bytecode','vm','value','math','constants','bindings','binding_item_resources_catalog','binding_spawn_catalog','binding_effect_resolve','binding_spawn_resolve','binding_item_model_resolve','model_metadata','binding_article_reference_resolve','binding_signal_resolve','compile','binding_item_resources_resolve','pose_metadata','moves','move_lowering','events','event_payload','storage','execution','runtime','receiver','binding_receiver','actor_access','binding_article_feedback']
 units=[ROOT/'src/melee/ft'/('native_script_'+n+'.c') for n in names]+[ROOT/'src/melee/ft'/n for n in ['native_callback_lifetime.c','native_fighter_pose_path.c','native_script_effect_dispatch.c']]
 units += [ROOT/'src/melee/it'/(n+'.c') for n in ['native_script_item_actor','native_script_item_snapshot','native_article_animation_demand', 'native_article_animation_texture']]
 units += [ROOT/'src/melee/gm'/(n+'.c') for n in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(n+'.c') for n in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage']]
 units += sorted(f for f in (ROOT/'tests/native').glob('script_bindings_unavailable_*.c') if f.name not in ['script_bindings_unavailable_article_feedback.c','script_bindings_unavailable_item_birth.c','script_bindings_unavailable_item.c','script_bindings_unavailable_presentation.c'])
 units += [HERE.parent/'script_effect_actor_absent.c',HERE.parent/'script_receiver_unavailable_identity.c',HERE.parent/'script_item_actor/boundary.c',HERE/'test.c']
 inputs=units+[HERE/'run.py',HERE/'README.md',ROOT/'docs/scripting/article_models.md',ROOT/'examples/popo_full.fms',ROOT/'examples/nana_full.fms',ROOT/'tools/native_fighter_item_script.c',ROOT/'src/melee/it/native_article_model.c',ROOT/'src/melee/it/item.c',ROOT/'src/sysdolphin/baselib/objalloc.c',a.compiler,HERE.parent/'script_form_feedback/extract.py',HERE.parent/'script_position_effect_abi/extract.py']+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))+list(out.rglob('*.inc'));hashes.update({str(f):sha(f) for f in inputs})
 runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 flags=['-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out)]
 commands=[]
 stages=[('strict-backend',[str(a.compiler),*flags,'-Wall','-Wextra','-Werror','-Wno-array-parameter','-fsyntax-only',str(ROOT/'src/melee/ft/native_script_binding_article_feedback.c'),str(ROOT/'src/melee/ft/native_script_binding_item_model_resolve.c')]),('runtime',[str(a.compiler),*flags,'-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]),('compile',[str(a.compiler),*flags,'-fuse-ld=lld','-ffunction-sections','-fdata-sections','-Wl,/opt:ref',*map(str,units),str(out/'runtime.obj'),'-o',str(out/'test.exe')]),('fixture',[str(out/'test.exe')])]
 for stage,cmd in stages:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-12000:])
 for f,h in hashes.items():
  if sha(Path(f))!=h:raise RuntimeError('input changed '+f)
 receipt={'status':'PASS','summary':json.loads(r.stdout),'native_reference':REFERENCE,'input_sha256':hashes,'commands':commands,'actual':['source compiler/catalog/bytecode/VM, Item actor/storage and native physical pool','prepared common Item model selector paths and direct Article feedback binding','unchanged async enqueue/process/flush/clear/init; eight exact sync/alternate cases and facing helper/refcounted generator attachment'],'controlled':['controlled imported primary Article descriptor and userdata construction, exact owned spec provider','fixed sixteen-packet queue allocator; generator asset/allocation and AppSRT allocation leaves','bounded immediate resource-leaf mutation probes; no authored callback exists in this native particle path','joint geometry sampler and typed position/scalar recipe captures; the two particle/model constructors do not run'],'limits':['no particle commands, renderer, game or imported asset appearance acceptance','queued joint is borrowed; later arbitrary graph replacement is not contained by this API','production ObjAlloc can grow backing heap; fixed fixture capacity is not allocation-free tick evidence','source rejection after immediate spawn does not undo native particle side effects']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
