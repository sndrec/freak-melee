"""Actual source-owned animation jump entry, phase execution and snapshots."""
import argparse, hashlib, importlib.util, json, subprocess, sys
from pathlib import Path
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
HERE = ROOT / 'tests/native/script_animation_jump'
BASE = ROOT / 'tests/native/script_fighter_presentation_scheduler'

def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args(); out = args.output.resolve()
    if out.drive.upper() != 'A:' or out.exists(): parser.error('Fresh A: output required')
    out.mkdir(parents=True)
    extractor = module('animation_jump_bodies', ROOT/'tests/native/script_form_feedback/extract.py')
    body = extractor.body
    sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    hashes = {}
    def source(relative):
        path = ROOT/relative; hashes[str(path)] = sha(path); return path.read_text()
    boundary = (BASE/'boundary.c').read_text()
    boundary = boundary.replace(body(boundary, 'nativeFighterRuntime_SourcePackage'), '')
    boundary = boundary.replace(body(boundary, 'Fighter_ChangeMotionState'), '')
    boundary = boundary.replace('#include "actual_presentation.h"', '')
    boundary = boundary.replace('presentation_bank.', 'animation_package.compiler.bank.')
    boundary = boundary.replace('registered=%lu', 'registered=%u')
    boundary = boundary.replace(r'#include <stdlib.h>\n#include <stdio.h>\n#include <stdarg.h>', '#include <stdlib.h>\n#include <stdio.h>\n#include <stdarg.h>')
    (out/'boundary.c').write_text('#include "fixture.h"\n'+boundary)
    props = source('src/melee/ft/native_script_binding_properties.c')
    context = '\n'.join(body(props, name) for name in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_Context','nativeScriptBindings_SourceFighterLayout'])
    (out/'actual_context.h').write_text(context)
    jump = source('src/melee/ft/kinds/ftCommon/ftCo_JumpAerial.c')
    movement = source('src/melee/ft/ftmovement.c')
    vertical = source('src/melee/ft/ft_084E.c')
    pieces = [body(jump, name) for name in ['ft_SetVec','ftCo_JumpAerial_Begin','ftCo_JumpAerial_Phys_Cb']]
    pieces += [body(movement, name) for name in ['ftCommon_CalcSelfAccel_Deaccel','ftCommon_CalcSelfAccel_AccelToVelClampedFrom','ftCommon_CalcSelfAccel_Drift','ftCommon_CalcSelfAccel_DriftFrom']]
    pieces += [body(vertical,'ft_800851D0')]
    (out/'actual_jump.inc').write_text('\n'.join(pieces))
    kinetics = source('src/melee/ft/native_script_binding_kinetics.c')
    invoke = body(kinetics,'nativeScriptBindingKinetics_Invoke')
    first = invoke.index('    switch(semantic) {'); last = invoke.index('\n    if(guarded &&',first)
    exact = '    case NativeScriptBinding_PhysicsAirAccelNormal:ftCommon_CalcSelfAccel_Drift(fighter);break;'
    assert exact in invoke
    invoke = invoke[:first]+'    switch(semantic) {\n'+exact+'\n    default:return error(diagnostic,"Unselected kinetic operation in animation jump fixture.");\n    }'+invoke[last:]
    (out/'kinetics.c').write_text('#include "fixture.h"\n#include <melee/ft/ftcommon.h>\n#include <melee/ft/ftanim.h>\n#include <melee/lb/lbanim.h>\n'+body(kinetics,'error')+'\n'+invoke)
    unavailable = (BASE/'unavailable.c').read_text().replace('CALL(nativeScriptBindingSetup_Invoke)','').replace('CALL(nativeScriptBindingKinetics_Invoke)','')
    (out/'unavailable.c').write_text(unavailable)
    names = ['source','expression','statement','program','bytecode','vm','value','math','constants','bindings', 'binding_item_resources_catalog', 'binding_spawn_catalog','compile', 'binding_item_resources_resolve','metadata','item_program', 'resource_metadata','fighter_program','fighter_actor','fighter_hooks','effect_program','moves','move_lowering','events','event_payload','storage','execution','runtime','receiver','binding_receiver','actor_access','pose_metadata','binding_spawn_resolve','binding_effect_resolve','binding_signal_resolve','binding_article_reference_resolve','binding_item_model_resolve','model_metadata','binding_setup','binding_jump','binding_air_physics']
    units = [ROOT/'src/melee/ft'/('native_script_'+name+'.c') for name in names]
    units += [ROOT/'src/melee/ft'/name for name in ['native_callback_lifetime.c','native_fighter_pose_path.c','native_fighter_pool.c','native_source_declarations.c','native_source_asset_validation.c','native_source_motion_policy.c','ftcharacter_asset_layout.c','native_fighter_sound_policy.c']]
    units += [ROOT/'src/melee/gm'/(name+'.c') for name in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
    units += [ROOT/'src/sysdolphin/baselib'/(name+'.c') for name in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage','dat']]
    units += sorted(p for p in (ROOT/'tests/native').glob('script_bindings_unavailable_*.c') if p.name not in {'script_bindings_unavailable_jump.c','script_bindings_unavailable_air_physics.c'})
    units += [ROOT/'tests/native/script_receiver_unavailable_identity.c',HERE/'test.c',out/'boundary.c',out/'unavailable.c',out/'kinetics.c']
    inputs = units+[Path(__file__),args.compiler,BASE/'boundary.c',BASE/'unavailable.c',BASE/'boundary.h',ROOT/'tests/native/script_form_feedback/extract.py']+list(HERE.glob('*'))+list(out.glob('*.h'))+list(out.glob('*.inc'))+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
    hashes.update({str(path):sha(path) for path in inputs})
    runtime = ROOT/'src/melee/ft/native_script_runtime.c'; units.remove(runtime)
    flags = ['-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),'-I'+str(HERE),'-I'+str(BASE)]
    commands = []
    for stage,command in [('runtime',[str(args.compiler),*flags,'-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]),('compile',[str(args.compiler),*flags,'-ffunction-sections','-fdata-sections','-fno-zero-initialized-in-bss','-fuse-ld=lld','-Wl,/opt:ref','-Wl,/errorlimit:0',*map(str,units),str(out/'runtime.obj'),'-o',str(out/'test.exe')]),('fixture',[str(out/'test.exe'),str(HERE/'scenario.fms')])]:
        result = subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW)
        (out/(stage+'.log')).write_text(result.stdout+result.stderr)
        commands.append({'stage':stage,'command':command,'exit':result.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
        if result.returncode: raise RuntimeError(stage+': '+(result.stdout+result.stderr)[-14000:])
    if any(sha(Path(path))!=digest for path,digest in hashes.items()):raise RuntimeError('Input changed during fixture')
    receipt={'status':'PASS','result':json.loads(result.stdout),'input_sha256':hashes,'commands':commands,'actual':['complete source compiler, prepared hook/motion records, actor/Execution/VM/catalog/Setup/Jump/AirPhysics','native GObj/Fighter pools and actor snapshots','mechanically extracted actual Begin, common drift and adjusted TransN physics kernel'],'controlled':['owned package facts/data and motion selection boundary','sound leaves instrument ordered entry, nested phase dispatch, actual VM fault and physical free','Kinetics switch narrowed mechanically to actual air_accel_normal case; unrelated operations reject'],'limits':['no retail animation/DAT/package/graphics or gameplay acceptance','snapshot restores actual actor records against live unchanged native peers; no full scene rollback claim','native audio does not normally invoke these test-injected script faults or frees']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['result']))
if __name__=='__main__':main()
