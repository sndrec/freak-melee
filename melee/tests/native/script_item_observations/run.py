"""Real typed Item observation calls and native sweep/animation comparisons."""
import argparse,hashlib,importlib.util,json,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 out.mkdir(parents=True);extract_path=ROOT/'tests/native/script_item_spawn/extract.py';spec=importlib.util.spec_from_file_location('extract',extract_path);extract=importlib.util.module_from_spec(spec);spec.loader.exec_module(extract)
 original=ROOT/'tests/native/script_compile/native_boundaries.c';absent=out/'unavailable.c'
 absent.write_text('#include <melee/ft/native_script_bindings.h>\n'+'\n'.join(extract.body(original.read_text(),n) for n in ['nativeScriptBindings_Context','nativeScriptBindingKinetics_Invoke','nativeScriptBindingState_Invoke','nativeScriptBindingSetup_Invoke','nativeScriptBindingCombat_Invoke','nativeScriptBindingTransition_Invoke'])+'\n')
 ground=ROOT/'src/melee/it/itgroundcoll.c';mapping=ROOT/'src/melee/it/itmaplib.c';animation=ROOT/'src/melee/it/it_2725.c';lb=ROOT/'src/melee/lb/lb_00B0.c';classes=ROOT/'src/sysdolphin/baselib/class.c';vector=ROOT/'src/melee/lb/lbvector.c';joint=ROOT/'src/sysdolphin/baselib/jobj.c';collision=ROOT/'src/melee/mp/mpcoll.c';trail=ROOT/'src/melee/it/kinds/itfoxillusion.c'
 sweep=extract.body(ground.read_text(),'it_8026DAA8').replace('return test;', 'observation(NativeScriptBinding_ItemSweepStageFlags); return test;')
 bodies=[sweep]
 for path,names in [(mapping,['it_80276214','it_80276308','it_802763E0','it_80276408','it_8027649C_ref','it_802765BC','it_80276CB8','it_8027770C']), (ground,['it_8026E0F4','it_8026E9A4','it_8026DA70']), (trail,['itFoxIllusion_Logic14_DmgDealt']), (animation,['it_80272C6C','it_80272CC0','it_80272980']), (lb,['lb_8000B09C']), (classes,['hsdObjIsDescendantOf']), (vector,['lbVector_Angle','lbVector_Mirror']), (joint,['HSD_JObjCheckDepend','HSD_JObjSetMtxDirtySub']), (collision,['mpCollSetFacingDir'])]:
  for name in names:
   body=extract.body(path.read_text(),name)
   # Explicit post-complete observation boundary; this does not claim a native callback.
   if name in ['it_80276308','it_802763E0','it_8027770C']:
    result={'it_80276308':'ret_val','it_802763E0':'result','it_8027770C':'ret_val'}[name]
    semantic={'it_80276308':'ItemWallContactFlags','it_802763E0':'ItemCeilingContactFlags','it_8027770C':'ItemReflectFromWall'}[name]
    body=body.replace('return '+result+';','observation(NativeScriptBinding_'+semantic+'); return '+result+';')
   if name in ['it_8026E0F4','it_80276CB8','it_80272980']:
    semantic={'it_8026E0F4':'ItemSweepFloor','it_80276CB8':'ItemAlignToFloor','it_80272980':'ItemAlignFacingToVelocity'}[name]
    at=body.rfind('}');body=body[:at]+'    observation(NativeScriptBinding_'+semantic+');\n'+body[at:]
   bodies.append(body)
 (out/'native_bodies.inc').write_text('\n'.join(bodies)+'\n')
 names=['pose_metadata','compile', 'binding_item_resources_resolve','moves','move_lowering','binding_signal_resolve','binding_article_reference_resolve','binding_item_model_resolve','model_metadata','binding_item_observations','binding_spawn_resolve','binding_effect_resolve','runtime','receiver','binding_receiver','constants','execution','event_payload','events','storage','bindings', 'binding_item_resources_catalog', 'binding_spawn_catalog','value','math','expression','source','statement','program','bytecode','vm']
 units=[ROOT/'src/melee/ft'/('native_script_'+n+'.c') for n in names]+[ROOT/'src/melee/ft/native_callback_lifetime.c',ROOT/'src/melee/ft/native_fighter_pose_path.c',ROOT/'src/melee/gm/native_match_pool.c',HERE/'test.c',HERE/'unselected_dependencies.c',absent]
 units+=sorted(f for f in (ROOT/'tests/native').glob('script_bindings_unavailable_*.c') if f.name!='script_bindings_unavailable_item_observations.c')
 units += [ROOT/'tests/native/script_receiver_unavailable_identity.c',ROOT/'tests/native/script_source_fighter_absent.c']
 units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c']
 registry=ROOT/'tests/native/script_receiver_unavailable_registry.c';extra=out/'registry.c';extra.write_text('#include <melee/ft/native_fighter_runtime.h>\n#include <melee/ef/native_model_effect.h>\n'+'\n'.join(extract.body(registry.read_text(),n) for n in ['nativeFighterRuntime_Definition','nativeModelEffect_Snapshot','nativeModelEffect_Resolve'])+'\n');units.append(extra)
 units.append(ROOT/'src/melee/lb/native_float_math.c')
 inputs=units+list(HERE.glob('*.h'))+[original,trail,ground,mapping,animation,lb,classes,vector,joint,collision,registry,HERE/'run.py',HERE/'README.md',ROOT/'docs/scripting/item_observations.md',extract_path,a.compiler.resolve()]+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 hashes={str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in inputs}
 command=[str(a.compiler),'-std=c11','-O2','-g','-ffp-contract=off','-fuse-ld=lld','-Wall','-Wextra','-Werror','-Wno-array-parameter','-Wno-missing-field-initializers','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),'-include',str(ROOT/'src/melee/lb/native_float_math.h'),*map(str,units),'-o',str(out/'test.exe')];commands=[]
 for stage,cmd in [('compile',command),('run',[str(out/'test.exe')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-11000:])
 for f,h in hashes.items():
  if hashlib.sha256(Path(f).read_bytes()).hexdigest()!=h:raise RuntimeError('input changed '+f)
 receipt={'status':'PASS','result':json.loads(r.stdout),'commands':commands,'input_sha256':hashes,'extracted_sha256':{'native_bodies.inc':hashlib.sha256((out/'native_bodies.inc').read_bytes()).hexdigest()},'actual':['complete typed Source/Program/Bytecode compilation and VM catalog dispatch','real Item lifetime/schema/source guards and fixed physical GObj pool +registry','mechanically extracted complete stage/floor sweeps, wall/ceiling publication, wall reflection, floor alignment and facing kernels; full Item and four-JObj payload comparisons','actual lbVector_Mirror/lbVector_Angle, it_80272CC0, mpCollSetFacingDir and HSD JObj matrix-dirty propagation','actual it_80272C6C/lb_8000B09C animation traversal and HSD class ancestry','all twenty-three authored helpers execute through actual VM; unchanged results after injected logical/physical retirement and schema/model changes','class admission reentry verifies source/root/class/extent before native traversal and floor alignment'],'controlled':['actor attachment/spec/status/view and descriptor publication are explicit fixture boundaries, not actual Item actor scheduler','mpColl_800471F8/mpColl_800477E0/mpCheckAllRemap stage geometry and it_80276278 adjustment leaf','operation-specific post-complete native identity observer models publication hazard; no authored stage or vector callback is asserted','class metadata initializer injects retirement/class changes through the actual ancestry initialization branch'],'warning_exclusions':['unused parameter, unused native stack pad and boolean switch warnings only inside mechanically extracted native body include','array-parameter Dolphin prototype and missing-field-initializers exclusions retain existing runner policy; production backend otherwise compiles with Wall/Wextra/Werror'],'limits':['no DAT import/stage collision geometry/gameplay acceptance','controlled JObj class/extent does not prove allocator bounds or arbitrary dangling-pointer detection','joint_world_position, attachment matrix rotation, floor friction and secondary resource selectors remain outside this fixture; unselected matrix/material dependencies abort if reached']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['result']))
if __name__=='__main__':main()
