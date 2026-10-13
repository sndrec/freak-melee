"""Actual reserved HSD records and allocator save/load, no game or GPU launch."""
import argparse, hashlib, json, subprocess, sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.dont_write_bytecode = True
sys.path.insert(0,str(HERE.parent/'hsd_reservation'))
from extract import function

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--benchmark',action='store_true',help='time retained-reference queries and particle captures in the controlled fixture')
    args = parser.parse_args()
    out = args.output.resolve()
    if out.drive.upper() != 'A:' or out.exists(): parser.error('fresh A: archive required')
    out.mkdir(parents=True)
    base = ROOT/'src/sysdolphin/baselib'
    units = [base/(n+'.c') for n in ['native_hsd_reservation','class','native_hsd_arena','objalloc','memory','hash','id']]
    units += [ROOT/'src/melee/gm'/(n+'.c') for n in ['native_match_pool','native_match_snapshot_pool','native_match_arena','native_match_scene','native_match_os_heap']]
    units += [ROOT/'src/melee/cm'/(n+'.c') for n in ['camera_state','native_camera_snapshot']]
    units += [ROOT/'src/melee/gr'/(n+'.c') for n in ['stage_state','native_stage_snapshot','native_ground_pool','stage_hazard_state','native_stage_hazard_snapshot','native_green_grid','native_green_grid_snapshot']]
    units += [ROOT/'src/melee/it/native_item_pool.c']
    units += [ROOT/'src/melee/gr/stage_start_state.c']
    units += [ROOT/'src/melee/if/ifall_state.c']
    units += [ROOT/'src/melee/if/ifnametag_state.c']
    units += [ROOT/'src/melee/gr/stage_generator_state.c',ROOT/'src/melee/gr/native_stage_generator_snapshot.c',ROOT/'src/melee/gr/stage_spawn_state.c',ROOT/'src/melee/gr/native_stage_spawn_snapshot.c']
    units += [ROOT/'src/melee/lb/dynamics_state.c',ROOT/'src/melee/lb/native_dynamics_snapshot.c']
    units += [ROOT/'src/melee/gm/gmvs_state.c',ROOT/'src/melee/gm/native_versus_control_snapshot.c',ROOT/'src/melee/it/itspawn_state.c',ROOT/'src/melee/if/ifstatus_state.c']
    units += [ROOT/'src/melee/ef/efdata.c',ROOT/'src/melee/ef/native_effect_control_snapshot.c']
    units += [base/(n+'.c') for n in ['particle_state','psdisp_state','native_particle_control_snapshot','lobj_state','native_light_control_snapshot','psappsrt','list']]
    units += [base/'native_material_texture_snapshot.c']
    units += [HERE/'test.c']
    units += [HERE/'camera_endpoints.c']
    car=ROOT/'src/melee/gr/grmutecity.c'
    sort_source='#include <melee/gr/stage_hazard_state_internal.h>\n#include <melee/gr/grmutecity.h>\nvoid grMuteCity_801F106C(s32 i) {(void)i;}\n'
    sort_source+='\n'.join(function(car.read_text(),n) for n in ['grMc_DistanceSquared','grMuteCity_801F1328'])
    (out/'car_sort.c').write_text(sort_source+'\n')
    units += [out/'car_sort.c']
    generator=ROOT/'src/melee/gr/grzakogenerator.c'
    gen_source='#include <melee/gr/grzakogenerator.h>\n#include <melee/gr/stage_generator_state.h>\n#include <melee/gr/stage_spawn_state.h>\n#include <string.h>\n#include <melee/lb/lb_00B0.h>\n#include <melee/gr/grlib.h>\n#include <sysdolphin/baselib/psstructs.h>\n#include <sysdolphin/baselib/random.h>\n#include <melee/gr/native_stage_generator_snapshot.h>\n#include <melee/it/inlines.h>\n#include <melee/it/it_26B1.h>\n#include <melee/it/itzako.h>\n#include <sysdolphin/baselib/gobj.h>\n#include <sysdolphin/baselib/gobjproc.h>\n'
    gen_source+='grZakoGenerator_Config*\n'+function(generator.read_text(),'grZakoGenerator_801CA394')+'\n'
    gen_source+='static inline void\n'+function(generator.read_text(),'grZakoGenerator_801CA43C_inline')+'\n'
    gen_source+='\n'.join(function(generator.read_text(),n) for n in ['grZakoGenerator_801CA43C','grZakoGenerator_801CAE04','grZakoGenerator_801CAEB0','grZakoGenerator_801CAEF0','grZakoGenerator_801CAC14','grZakoGenerator_801CACB8'])
    (out/'generator_consumer.c').write_text(gen_source+'\n')
    units += [out/'generator_consumer.c']
    rainbow=ROOT/'src/melee/gr/grrcruise.c'
    rc=rainbow.read_text();start=rc.index('s16 grRc_803E4FF0[]')
    rc_source='#include <melee/gr/grrcruise.h>\n#include <melee/gr/grrcruise_data.h>\n#include <melee/gr/stage_hazard_state_internal.h>\n#include <melee/gr/ground.h>\n#include <melee/gr/stage.h>\n#include <melee/gr/inlines.h>\n#include <melee/mp/mplib.h>\n#include <sysdolphin/baselib/jobj.h>\n#include <sysdolphin/baselib/random.h>\n'
    rc_source+=rc[start:rc.index('};',start)+2]+'\n'
    rc_source+='\n'.join(function(rc,n) for n in ['grRCruise_80200B48','grRCruise_80200C04','grRCruise_802010A4'])
    (out/'rainbow_consumer.c').write_text(rc_source+'\n');units += [out/'rainbow_consumer.c']
    effects=ROOT/'src/melee/ef/eflib.c'
    ef_source='#include <melee/ef/eflib.h>\n#include <melee/ef/efdata.h>\n'+'\n'.join(function(effects.read_text(),n) for n in ['efLib_SetParamAlpha','efLib_SetParamGfxId'])
    (out/'effect_consumer.c').write_text(ef_source+'\n');units += [out/'effect_consumer.c']
    generator_id=ROOT/'src/sysdolphin/baselib/generator.c'
    (out/'generator_id.c').write_text('#include <sysdolphin/baselib/particle_state_internal.h>\n'+function(generator_id.read_text(),'hsd_8039D1EC')+'\n')
    units += [out/'generator_id.c']
    lighting=base/'lobj.c'
    light_source='#include <sysdolphin/baselib/lobj_state_internal.h>\n#include <sysdolphin/baselib/object.h>\n'
    light_source+='\n'.join(function(lighting.read_text(),n) for n in ['HSD_LObjAddCurrent','HSD_LObjDeleteCurrent','HSD_LObjUnrefThis'])
    (out/'light_consumer.c').write_text(light_source+'\n');units += [out/'light_consumer.c']
    material=base/'mobj.c';material_text=material.read_text()
    material_source='#include <sysdolphin/baselib/mobj.h>\n#include <sysdolphin/baselib/debug.h>\n'
    material_source+='\n'.join(line for line in material_text.splitlines() if line in
                              ['HSD_TObj* tobj_shadows;','HSD_TObj* tobj_toon;'])+'\n'
    material_source+='\n'.join(function(material_text,n) for n in
                              ['HSD_MObjAddShadowTexture','HSD_MObjDeleteShadowTexture'])
    (out/'material_consumer.c').write_text(material_source+'\n');units += [out/'material_consumer.c']
    display=base/'psdisp.c'
    display_source='#include <sysdolphin/baselib/psdisp.h>\n#include <sysdolphin/baselib/psdisp_state_internal.h>\n#include <sysdolphin/baselib/particle_state_internal.h>\n#include <string.h>\ntypedef struct { HSD_Particle* head; HSD_Particle* tail; } psdisp_ParticleSortBucket;\n'
    (out/'particle_sort.c').write_text(display_source+function(display.read_text(),'particleSort')+'\n')
    units += [out/'particle_sort.c']
    os_source = ROOT.parent/'aurora/lib/dolphin/os/OSAlloc.cpp'
    os = os_source.read_text().replace(function(os_source.read_text(),'OSDumpHeap'),'')
    os = os.replace('#include "../../logging.hpp"','').replace('static aurora::Module AllocLog("aurora::os::alloc");','')
    (out/'OSAlloc.cpp').write_text(os)
    inputs = units + [HERE/'snapshot_cases.h',HERE/'objalloc_cases.h',HERE/'objalloc_heap_cases.h',HERE/'class_cases.h',HERE/'class_arena_cases.h',HERE/'camera_cases.h',HERE/'stage_cases.h',HERE/'hazard_cases.h',HERE/'bubble_cases.h',HERE/'moving_cases.h',HERE/'grid_cases.h',HERE/'car_wind_cases.h',HERE/'pura_cases.h',HERE/'big_blue_cases.h',HERE/'generator_cases.h',HERE/'spawn_cases.h',HERE/'rainbow_cases.h',HERE/'fountain_cases.h',HERE/'effect_control_cases.h',HERE/'particle_control_cases.h',HERE/'versus_control_cases.h',HERE/'reference_benchmark.h',HERE/'id_cases.h',HERE/'os_heap_cases.h',HERE/'run.py',HERE/'README.md',HERE.parent/'hsd_reservation/extract.py',os_source,out/'OSAlloc.cpp',car,generator,generator_id,rainbow,effects,ROOT/'src/melee/gr/grizumi.c',args.compiler]
    inputs += [display,lighting,HERE/'light_control_cases.h',HERE/'scene_history_cases.h']
    inputs += [material,HERE/'material_texture_cases.h']
    inputs += list(base.rglob('*.h')) + list((ROOT/'src/Runtime').rglob('*.h'))
    inputs += list((ROOT/'extern/dolphin/include').rglob('*.h')) + list((ROOT.parent/'aurora/include').rglob('*.h'))
    inputs += [ROOT/'src/melee/gm'/(n+'.h') for n in ['native_match_pool','native_match_snapshot_pool','native_match_arena']]
    inputs += list((ROOT/'src/melee').rglob('*.h'))
    sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    hashes = {str(p):sha(p) for p in inputs}
    commands=[]
    def run(stage,command):
        result = subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
        (out/(stage+'.log')).write_text(result.stdout+result.stderr)
        commands.append({'stage':stage,'command':command,'exit':result.returncode})
        (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
        if result.returncode: raise RuntimeError(stage+': '+(result.stdout+result.stderr)[-6000:])
        return result.stdout
    flags=['-std=c11','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-I'+str(ROOT/'src'),'-I'+str(base),'-I'+str(ROOT/'extern/dolphin/include')]
    if args.benchmark: flags += ['-DNATIVE_SNAPSHOT_BENCHMARK=1']
    run('strict',[str(args.compiler),*flags,'-Wall','-Wextra','-Werror','-Wno-array-parameter','-fsyntax-only',str(units[0]),str(ROOT/'src/melee/cm/native_camera_snapshot.c'),str(ROOT/'src/melee/gr/native_stage_snapshot.c'),str(ROOT/'src/melee/gr/native_stage_hazard_snapshot.c'),str(ROOT/'src/melee/gr/native_green_grid_snapshot.c'),str(ROOT/'src/melee/gr/native_stage_spawn_snapshot.c'),str(ROOT/'src/melee/ef/native_effect_control_snapshot.c'),str(base/'native_particle_control_snapshot.c'),str(ROOT/'src/melee/gm/native_versus_control_snapshot.c'),str(HERE/'test.c'),str(HERE/'camera_endpoints.c')])
    run('native-os',[str(args.compiler),'-std=c++17','-O2','-ffunction-sections','-fdata-sections','-DTARGET_PC','-DAURORA','-I'+str(ROOT.parent/'aurora/include'),'-c',str(out/'OSAlloc.cpp'),'-o',str(out/'OSAlloc.obj')])
    run('compile',[str(args.compiler),*flags,'-O2','-ffunction-sections','-fdata-sections','-fuse-ld=lld','-Wl,/opt:ref',*map(str,units),str(out/'OSAlloc.obj'),'-o',str(out/'test.exe')])
    summary=json.loads(run('fixture',[str(out/'test.exe')]))
    for mode in ['allocation','free','geometry']:
        result=subprocess.run([str(out/'test.exe'),'frozen-'+mode],cwd=ROOT,capture_output=True,text=True,timeout=30,creationflags=subprocess.CREATE_NO_WINDOW)
        (out/('frozen-'+mode+'.log')).write_text(result.stdout+result.stderr)
        commands.append({'stage':'frozen-'+mode,'exit':result.returncode})
        if result.returncode==0 or '[os-heap]' not in result.stderr or 'refused during retained history' not in result.stderr:
            raise RuntimeError('frozen heap mutation was not rejected: '+mode)

    if any(sha(Path(p)) != digest for p,digest in hashes.items()): raise RuntimeError('source input changed')
    receipt={'status':'PASS','summary':summary,'source_sha256':hashes,'commands':commands,
             'actual':['direct HSD class-piece and ObjAlloc entry points; native reservation owner and pool serializers','actual Aurora OSAlloc bodies except diagnostic OSDumpHeap','generation reuse, full occupied bytes, exact free order and ended claim failure flag','late malformed owner and pool rejection without partial writes; overlap, active claim and retired epoch rejection','shared ObjAlloc descriptor state and fixed arena cursor, ordinary/reserved usage, limits and 256-descriptor ceiling; same-shape reinitialization invalidates snapshots','ordinary class blocks, entries and tables; initialized class counters; real new/delete, table growth/split/recycle and exact next allocation addresses'],
             'controlled':['caller-reserved backing and synthetic payloads; assert/report leaves terminate or print',
                           'camera cohort: GM idle predicate, synthetic CObj initialization and aborting unused graph/render/callback endpoints',
                           'stage cohort: controlled selected-stage getter/idle scheduler, eight-slot GObj pool, synthetic class joint/camera initialization; actual Ground owner and class companions restore'],
             'limits':['no graph constructor/render/gameplay here; connected scripted graph fixture supplies separate evidence','actual class/ObjAlloc components restore payloads/free links; actual Aurora OS component restores Cell/heap metadata; actual ID component restores default-table roots','class snapshots require the initialized class tree frozen; direct HSD allocations, other globals and aggregate ownership remain open','OS scratch is fixed and snapshot validation sorts current ownership; no eight-actor or total timing proof','no aggregate snapshot or whole-match timing claim']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(summary))

if __name__ == '__main__': main()
