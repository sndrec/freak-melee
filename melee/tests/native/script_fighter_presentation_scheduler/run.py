"""Actual shared presentation schedules and interpreted Fighter lifecycle."""
import argparse,hashlib,importlib.util,json,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
from extract import extract
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 out.mkdir(parents=True);hashes=extract(ROOT,out/'actual_presentation.h')
 names=['source','expression','statement','program','bytecode','vm','value','math','constants','bindings', 'binding_item_resources_catalog', 'binding_spawn_catalog','compile', 'binding_item_resources_resolve','metadata','item_program', 'resource_metadata','fighter_program','fighter_actor','fighter_hooks','effect_program','moves','move_lowering','events','event_payload','storage','execution','runtime','receiver','binding_receiver','actor_access','pose_metadata','binding_spawn_resolve','binding_effect_resolve','binding_signal_resolve','binding_article_reference_resolve','binding_item_model_resolve','model_metadata']
 units=[ROOT/'src/melee/ft'/('native_script_'+n+'.c') for n in names]+[ROOT/'src/melee/ft/native_callback_lifetime.c',ROOT/'src/melee/ft/native_fighter_pose_path.c',ROOT/'src/melee/ft/native_fighter_pool.c',ROOT/'src/sysdolphin/baselib/dat.c']
 units += [ROOT/'src/melee/gm'/(n+'.c') for n in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(n+'.c') for n in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage']]
 units += sorted((ROOT/'tests/native').glob('script_bindings_unavailable_*.c'))+[ROOT/'tests/native/script_receiver_unavailable_identity.c',HERE/'test.c',HERE/'boundary.c',HERE/'unavailable.c']
 units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c', ROOT/'src/melee/it/native_article_animation_origins.c', ROOT/'src/melee/ft/native_source_motion_policy.c', ROOT/'src/melee/ft/native_source_declarations.c', ROOT/'src/melee/ft/native_script_binding_item_animation_prepare.c']
 units += [ROOT/'src/melee/ft/native_source_asset_validation.c',ROOT/'src/melee/ft/native_fighter_sound_policy.c']
 units += [ROOT/'src/melee/ft/ftcharacter_asset_layout.c']
 inputs=units+[HERE/'run.py',HERE/'extract.py',HERE/'README.md',HERE/'boundary.h',HERE/'common_policy_cases.h',a.compiler]+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest();hashes.update({str(f):sha(f) for f in inputs})
 runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 common=['-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out)]
 commands=[]
 sequence=[('runtime',[str(a.compiler),*common,'-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]),('compile',[str(a.compiler),*common,'-ffunction-sections','-fdata-sections','-fno-zero-initialized-in-bss','-fuse-ld=lld','-Wl,/opt:ref','-Wl,/errorlimit:0',*map(str,units),str(out/'runtime.obj'),'-o',str(out/'test.exe')]),('fixture',[str(out/'test.exe')])]
 for stage,cmd in sequence:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode})
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-15000:])
 if any(sha(Path(f))!=h for f,h in hashes.items()):raise RuntimeError('input changed during fixture')
 receipt={'status':'PASS','result':json.loads(r.stdout),'input_sha256':hashes,'commands':commands,'actual':['shared color/presentation/demo/taunt bodies mechanically extracted without behavioral substitutions','actual Source/Program/Bytecode/VM/Fighter actor and root hook contracts','actual native Fighter/GObj pool allocation and callback registry'],'limits':['model/costume/camera/audio/common initialization and graphics leaves controlled','position-write boundary injects lifetime failures; source event bodies remain actual VM','no owned asset import, renderer, complete clone or game acceptance claim']};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['result']))
if __name__=='__main__':main()
