"""Hidden connected stored held-item visibility fixture."""
import argparse,hashlib,json,subprocess,importlib.util,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BASE=HERE.parent/'script_fighter_observations'
spec=importlib.util.spec_from_file_location('target_extract',BASE/'extract.py');extractor=importlib.util.module_from_spec(spec);spec.loader.exec_module(extractor)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('Fresh A: output required')
 out.mkdir(parents=True);hashes=extractor.extract(ROOT,out)
 # Reuse controlled engine leaves only. Production properties/Context/guards
 # execute from their complete translation unit instead of fixture routes.
 spec_body=importlib.util.spec_from_file_location('body',BASE.parent/'script_form_feedback/extract.py');body_module=importlib.util.module_from_spec(spec_body);spec_body.loader.exec_module(body_module)
 boundary=(BASE/'boundary.c').read_text()
 for name in ['nativeScriptBindings_Context','nativeScriptBindings_Read','nativeScriptBindings_Write']:
  boundary=boundary.replace(body_module.body(boundary,name),'')
 boundary=boundary.replace('#include "source_guards.inc"','')
 boundary += '\nbool hsdObjIsDescendantOf(HSD_Obj*o,HSD_ClassInfo*i){(void)o;(void)i;abort();}\nHSD_JObjInfo hsdJObj;\n'
 boundary += '\n#include <melee/ft/ftcommon_assets.h>\n#include <melee/ft/native_costume_runtime.h>\nstruct Fighter_804D653C_t* Fighter_804D653C;struct Fighter_804D653C_t* Fighter_804D6538;\nuint8_t nativeCostumeRuntime_ModelVariant(Fighter* f){(void)f;abort();}\nvoid ftParts_80074A4C(HSD_GObj* o,int a,int b){(void)o;(void)a;(void)b;abort();}\nvoid ftParts_80074B0C(HSD_GObj* o,int a,int b){(void)o;(void)a;(void)b;abort();}\nbool ftCo_800BFFD0(Fighter* f,int a,bool b){(void)f;(void)a;(void)b;abort();}\n'
 (out/'boundary.c').write_text(boundary)
 hashes[str(BASE/'boundary.c')]=hashlib.sha256((BASE/'boundary.c').read_bytes()).hexdigest()
 native=ROOT/'src/melee/ft/ftcommon.c';native_text=native.read_text();oracle=body_module.body(native_text,'ftCommon_8007F5CC')
 (out/'native_visibility.inc').write_text(oracle+'\n')
 module_source=ROOT/'src/melee/ft/native_fighter_module.c'
 (out/'native_common.inc').write_text(body_module.body(module_source.read_text(),'common_attribute')+'\n')
 hashes[str(module_source)]=hashlib.sha256(module_source.read_bytes()).hexdigest()
 hashes[str(native)]=hashlib.sha256(native.read_bytes()).hexdigest()
 exporter=ROOT/'tools/native_fighter_script_primitives.c'
 if '{ "visuals.held_item_visible", "fighter->x221E_b3" }' not in exporter.read_text():raise RuntimeError('Exporter held visibility mapping changed')
 hashes[str(exporter)]=hashlib.sha256(exporter.read_bytes()).hexdigest()
 names=['native_script_source','native_script_expression','native_script_statement','native_script_program','native_script_bytecode','native_script_vm','native_script_value','native_script_math','native_script_constants','native_script_bindings', 'native_script_binding_item_resources_catalog', 'native_script_binding_spawn_catalog','native_script_binding_effect_resolve','native_script_binding_spawn_resolve','native_script_compile', 'native_script_binding_item_resources_resolve','native_script_pose_metadata','native_fighter_pose_path','native_script_moves','native_script_move_lowering','native_script_events','native_script_event_payload','native_script_storage','native_script_execution','native_script_runtime','native_script_receiver','native_script_binding_receiver','native_script_actor_access','native_callback_lifetime','native_script_binding_visuals','native_script_binding_common_properties','native_script_binding_properties','native_script_binding_article_reference_resolve','native_script_binding_signal_resolve','native_script_fighter_actor','native_script_fighter_program','native_script_item_program', 'native_script_binding_item_animation_prepare', 'native_script_resource_metadata','native_script_effect_program','native_script_metadata']
 units=[ROOT/'src/melee/ft'/(name+'.c') for name in names]+[ROOT/'src/melee/it/native_script_item_actor.c',ROOT/'src/melee/it/native_script_item_snapshot.c']
 units += [ROOT/'src/melee/gm'/(name+'.c') for name in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(name+'.c') for name in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage','dat']]
 units += [HERE.parent/'script_effect_actor_absent.c',HERE.parent/'script_receiver_unavailable_identity.c',HERE/'test.c',out/'boundary.c']
 units += [p for p in sorted(HERE.parent.glob('script_bindings_unavailable_*.c')) if p.stem not in {'script_bindings_unavailable_presentation','script_bindings_unavailable_item','script_bindings_unavailable_visuals','script_bindings_unavailable_common_properties','script_bindings_unavailable_item_birth'}]
 units += [ROOT/'src/melee/ft/native_script_binding_item_model_resolve.c',ROOT/'src/melee/ft/native_script_model_metadata.c',ROOT/'src/melee/db/db_state.c']
 units += [ROOT/'src/melee/ft/native_script_fighter_hooks.c']
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_source_declarations', 'native_source_asset_validation', 'ftcharacter_asset_layout', 'native_fighter_sound_policy']]
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_source_motion_policy']]
 units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c',ROOT/'src/melee/it/native_article_animation_origins.c']
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
 inputs=[HERE/'fixture.h',BASE/'fixture.h',BASE/'extract.py',Path(__file__)]+units+list(HERE.glob('*.py'))+list(HERE.glob('*.h'))+[HERE/'README.md',a.compiler]+list(out.glob('*.inc'))+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 hashes.update({str(path):sha(path) for path in inputs})
 runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 flags=['-std=gnu11','-O2','-ffp-contract=off','-Werror','-Wno-array-parameter','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-g','-include',str(ROOT/'src/melee/ft/native_fighter_lifetime.h'),'-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),'-I'+str(HERE),'-I'+str(BASE),'-I'+str(ROOT/'src/melee/it')]
 commands=[]
 syntax=[str(a.compiler),'-std=c11','-Wall','-Wextra','-Werror','-Wno-array-parameter',*flags[3:],'-fsyntax-only',str(ROOT/'src/melee/ft/native_script_binding_visuals.c')]
 fixture_syntax=[str(a.compiler),*flags,'-Wall','-Wextra','-Wno-array-parameter','-fsyntax-only',str(HERE/'test.c')]
 compile=[str(a.compiler),*flags,'-fuse-ld=lld','-ffunction-sections','-fdata-sections','-Wl,/opt:ref',*map(str,units),str(out/'runtime.obj'),'-o',str(out/'test.exe')]
 runtime_command=[str(a.compiler),*flags,'-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]
 common_syntax=syntax[:-1]+[str(ROOT/'src/melee/ft/native_script_binding_common_properties.c')]
 for stage,cmd in [('syntax',syntax),('common-syntax',common_syntax),('fixture-syntax',fixture_syntax),('runtime',runtime_command),('compile',compile),('fixture',[str(out/'test.exe')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
  (out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-12000:])
 for path,digest in hashes.items():
  if sha(Path(path))!=digest:raise RuntimeError('Input changed: '+path)
 receipt={'status':'PASS','summary':json.loads(r.stdout),'source_sha256':hashes,'commands':commands,
 'actual':['frontend/catalog/compiler/Prepare/Publish/Fighter actor/Execution/VM/storage/real GObj pools','complete production visibility/common property readers and routing; native stored bit and actual common_attribute comparisons'],
 'controlled':['synthetic imported rig/package metadata and runtime package lookup; static Fighter payload allocation','unrelated engine leaves unavailable; Fighter payload scene bind boundary'],
 'limits':['no game/rendering/held visibility callback execution acceptance','stock, detached, malformed, stale and retired contexts reject; synthetic source assets only']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
