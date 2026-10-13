"""Source held callbacks through the actual native dispatcher/actor/VM/GObj."""
import argparse,hashlib,json,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]

def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 out.mkdir(parents=True);compiler=a.compiler.resolve()
 names=['source','expression','statement','program','bytecode','vm','value','math','constants','bindings', 'binding_item_resources_catalog', 'binding_spawn_catalog','binding_effect_resolve','binding_spawn_resolve','compile', 'binding_item_resources_resolve','moves','move_lowering','events','event_payload','storage','execution','runtime','receiver','binding_receiver','actor_access']
 units=[ROOT/'src/melee/ft'/('native_script_'+n+'.c') for n in names]+[ROOT/'src/melee/ft/native_callback_lifetime.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_article_feedback.c', ROOT/'tests/native/script_bindings_unavailable_item_resources.c', ROOT/'tests/native/script_bindings_unavailable_hitlag.c', ROOT/'tests/native/script_bindings_unavailable_stage.c', ROOT/'tests/native/script_bindings_unavailable_family.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_common_movement.c', ROOT/'tests/native/script_bindings_unavailable_visuals.c']
 units += [ROOT/'src/melee/ft/native_script_pose_metadata.c', ROOT/'src/melee/ft/native_fighter_pose_path.c', ROOT/'tests/native/script_bindings_unavailable_pose.c']
 units += [ROOT/'src/melee/it'/(n+'.c') for n in ['native_script_item_actor','native_script_item_snapshot','native_held_article']]
 units += [ROOT/'src/melee/gm'/(n+'.c') for n in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(n+'.c') for n in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage']]
 boundary=HERE.parent/'script_item_actor/boundary.c';text=boundary.read_text();needle='item->pos.x=value->as.f32;'
 if text.count(needle)!=1:raise RuntimeError('write injection boundary changed')
 text=text.replace('#include "boundary.h"','#include "boundary.h"\nextern void (*held_after_write)(HSD_GObj*);')
 text=text.replace(needle,needle+'if(held_after_write){void (*next)(HSD_GObj*)=held_after_write;held_after_write=NULL;next(object);}')
 (out/'boundary.c').write_text(text)
 units += [HERE.parent/'script_effect_actor_absent.c',HERE.parent/'script_bindings_unavailable_actions.c',HERE.parent/'script_bindings_unavailable_effect.c',HERE.parent/'script_bindings_unavailable_spawn.c',HERE.parent/'script_receiver_unavailable_identity.c',out/'boundary.c',HERE/'test.c']
 sha=lambda x:hashlib.sha256(x.read_bytes()).hexdigest();inputs=units+[HERE/'run.py',boundary,compiler]+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))+list((HERE.parent/'script_item_actor').glob('*.h'))
 hashes={str(x):sha(x) for x in inputs};commands=[]
 common=['-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(HERE.parent/'script_item_actor')]
 runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 stages=[('compile-runtime',[str(compiler)]+common+['-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]),
  ('compile',[str(compiler)]+common+['-ffunction-sections','-fdata-sections','-fno-zero-initialized-in-bss','-fuse-ld=lld','-Wl,/opt:ref']+list(map(str,units))+[str(out/'runtime.obj'),'-o',str(out/'test.exe')]),('fixture',[str(out/'test.exe')])]
 for stage,command in stages:
  r=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
  (out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':command,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+' failed:\n'+(r.stdout+r.stderr)[-8000:])
 if any(sha(Path(x))!=v for x,v in hashes.items()):raise RuntimeError('input changed')
 receipt={'status':'PASS','summary':json.loads(r.stdout),'commands':commands,'input_sha256':hashes,
  'boundaries':['actual held dispatcher, source compiler/event ABI, Item actor/storage/VM, native GObj pools/lifecycle','controlled publication specs, native Item userdata, absent Fighter authored view, position-write removal/state transition injection'],
  'limits':['no game launch or renderer','actual common held callsites audited but not executed by this fixture','source-only no NativeFighterDefinition or authored C callbacks']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
