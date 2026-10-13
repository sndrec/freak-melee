"""Hidden connected attachment existing-matrix reader cohort."""
import argparse,hashlib,importlib.util,json,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 out.mkdir(parents=True);s=importlib.util.spec_from_file_location('extract',HERE/'extract.py');e=importlib.util.module_from_spec(s);s.loader.exec_module(e);hashes=e.extract(ROOT,out)
 names=['source','expression','statement','program','bytecode','vm','value','math','constants','bindings','binding_item_resources_catalog','binding_spawn_catalog','compile','binding_item_resources_resolve','metadata','item_program','resource_metadata','fighter_program','fighter_actor','fighter_hooks','effect_program','moves','move_lowering','events','event_payload','storage','execution','runtime','receiver','binding_receiver','actor_access','pose_metadata','binding_spawn_resolve','binding_effect_resolve','binding_signal_resolve','binding_article_reference_resolve','binding_item_model_resolve','model_metadata','binding_item_observations']
 units=[ROOT/'src/melee/ft'/('native_script_'+n+'.c') for n in names]
 units += [ROOT/'src/melee/ft'/(n+'.c') for n in ['native_callback_lifetime','native_fighter_pose_path','native_source_declarations','native_source_asset_validation','native_fighter_sound_policy','ftcharacter_asset_layout','native_source_motion_policy','native_source_bank']]
 units += [ROOT/'src/melee/ft/native_script_binding_item_animation_prepare.c']
 units += [ROOT/'src/melee/it'/(n+'.c') for n in ['native_script_item_actor','native_script_item_snapshot','native_article_animation_demand', 'native_article_animation_texture','native_article_animation_origins']]
 units += [ROOT/'src/melee/gm'/(n+'.c') for n in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(n+'.c') for n in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage','dat']]
 units += sorted(f for f in (ROOT/'tests/native').glob('script_bindings_unavailable_*.c') if f.name not in ['script_bindings_unavailable_item_observations.c','script_bindings_unavailable_presentation.c','script_bindings_unavailable_item.c','script_bindings_unavailable_item_birth.c'])
 birth=ROOT/'tests/native/script_bindings_unavailable_item_birth.c';bs=importlib.util.spec_from_file_location('body',ROOT/'tests/native/script_form_feedback/extract.py');bm=importlib.util.module_from_spec(bs);bs.loader.exec_module(bm)
 text=birth.read_text();(out/'unavailable_birth.c').write_text(text.replace(bm.body(text,'nativeScriptItem_GetCreationValue'),''));units.append(out/'unavailable_birth.c')
 identity=ROOT/'tests/native/script_receiver_unavailable_identity.c'
 (out/'unavailable_identity.c').write_text(identity.read_text().replace(bm.body(identity.read_text(),'nativeItem_Creator'),''))
 units += [out/'unavailable_identity.c',ROOT/'tests/native/script_effect_actor_absent.c',HERE/'test.c',HERE/'boundary.c',out/'native_bodies.c']
 sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest()
 inputs=units+list(HERE.glob('*.py'))+list(HERE.glob('*.h'))+[HERE/'README.md',ROOT/'docs/scripting/article_models.md',ROOT/'docs/scripting/item_observations.md',ROOT/'examples/link_full.fms',ROOT/'examples/young_link_full.fms',a.compiler,birth,identity]+list(out.glob('*.inc'))+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 hashes.update({str(f):sha(f) for f in inputs});(out/'input_sha256.json').write_text(json.dumps(hashes,indent=2)+'\n')
 runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 flags=['-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(HERE),'-I'+str(out)]
 commands=[]
 stages=[('strict',[str(a.compiler),*flags,'-Wall','-Wextra','-Werror','-Wno-array-parameter','-fsyntax-only',str(ROOT/'src/melee/ft/native_script_binding_item_observations.c')]),('fixture-strict',[str(a.compiler),*flags,'-Wall','-Wextra','-Werror','-Wno-array-parameter','-fsyntax-only',str(HERE/'test.c'),str(HERE/'boundary.c')]),('runtime',[str(a.compiler),*flags,'-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]),('compile',[str(a.compiler),*flags,'-ffunction-sections','-fdata-sections','-fno-zero-initialized-in-bss','-fuse-ld=lld','-Wl,/opt:ref','-Wl,/errorlimit:0',*map(str,units),str(out/'runtime.obj'),'-o',str(out/'test.exe')]),('fixture',[str(out/'test.exe')])]
 for stage,cmd in stages:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-16000:])
 changed={f:{'before':h,'after':sha(Path(f))} for f,h in hashes.items() if sha(Path(f))!=h};(out/'changed_inputs.json').write_text(json.dumps(changed,indent=2)+'\n')
 if changed:raise RuntimeError('inputs changed')
 receipt={'status':'PASS','summary':json.loads(r.stdout),'native_reference':'948cf3ed5','input_sha256':hashes,'commands':commands,'actual':['compiler/catalog/Prepare/publication/VM and Fighter+Item actors/storage','physical GObj pools/free/reuse and immutable creator/mutable attachment relation','unchanged HSD_MtxGetRotation/HSD_MkRotationMtx and native creator bootstrap/class ancestry','stock/independent creators and exact raw xDC4 part; AlwaysAfterGuard output publication'], 'controlled':['selected packages/import assets and fixed userdata/JObj storage','unrelated observation/native/render branches explicitly abort or unavailable','class metadata initializer mutation checks physical/source/schema/rig admission'], 'limits':['no game/DAT/renderer or whole attachment gameplay acceptance','JObj class/extent and retained identity do not establish allocator-generation ownership or arbitrary destructive callback containment','matrix math has no authored callback and never invokes SetupMatrix']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
