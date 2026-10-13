"""Offline live component census, with observation-only gameplay comparison.

Runs stock and independent interpreted Ice Climbers pairs (eight actual actors).
The existing accepted family is copied into an isolated store; no user content
is changed. Optional same-boundary Load/recapture is not history rewind, complete
state closure, or rollback-deadline acceptance.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
from trace_interval import compare_interval, compare_correction

ROOT = Path(__file__).resolve().parents[3]
OWNERS = {'gobj', 'fighter', 'ground', 'item', 'item-link', 'animation',
          'objalloc', 'class', 'hsd-reservations', 'id', 'collision',
          'article-resources', 'fighter-vm', 'item-vm', 'effect-vm', 'selected-os-heap',
          'hsd-memory', 'robj-expression', 'controller', 'input-clock', 'rng', 'gm-loop', 'gm-input', 'player', 'camera-subjects', 'camera', 'stage', 'stage-hazards', 'green-grid', 'dynamics', 'stage-generator', 'stage-spawns', 'effect-controls', 'particle-controls','versus-controls','texture-identities','light-controls','stage-start','material-textures'}


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        while block := stream.read(1024 * 1024):
            digest.update(block)
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--family-receipt', type=Path, required=True)
    parser.add_argument('--samples', type=int, default=1200)
    parser.add_argument('--stage',default='final-destination',help='versus stage passed to the native match launcher')
    parser.add_argument('--self-restore',action='store_true',help='load this boundary’s captured components together, then require exact recaptures')
    parser.add_argument('--rewind',action='store_true',help='restore an eight-tick older own capture, compare it, then recover the complete current endpoint')
    parser.add_argument('--resimulate',action='store_true',help='repeat eight rendered input ticks from compact state, compare every owner, and recover the original endpoint')
    parser.add_argument('--correction',action='store_true',help='correct human P1 inputs, compare a fresh corrected run, then repeat the correction from compact state')
    parser.add_argument('--depth',type=int,default=8,choices=range(1,16),metavar='1..15')
    parser.add_argument('--rounds',type=int,default=1,choices=range(1,65),metavar='1..64')
    parser.add_argument('--history',action='store_true',help='capture every boundary into sixteen compact slots and restore through their frame directory')
    parser.add_argument('--before-input',action='store_true',help='capture at production pre-input phase, starting with boundary zero')
    parser.add_argument('--target',type=int,default=240,help='first retained controller boundary for rewind')
    parser.add_argument('--benchmark',action='store_true',help='time sixteen complete uninstrumented compact capture/load calls at each census checkpoint')
    parser.add_argument('--replay-timing',action='store_true',help='time complete rendered replay intervals and phases with trace/census instrumentation retained')
    parser.add_argument('--quiet-replay-timing',action='store_true',help='omit per-frame tracing/object census; measure same-input replay with compact/raw endpoint proof only')
    parser.add_argument('--profile-save',action='store_true',help='profile save phases and copy experiments; requires profiler build and quiet replay timing')
    parser.add_argument('--profile-save-detail',action='store_true',help='add intrusive per-root clocks; requires --profile-save')
    parser.add_argument('--skip-model-geometry',action='store_true',help='omit model primitives during restored ticks, retaining source callbacks and matrix setup')
    parser.add_argument('--skip-gpu-output',action='store_true',help='also omit GPU frame allocation, uploads and submission during restored ticks')
    args = parser.parse_args()
    if args.profile_save and not args.quiet_replay_timing:
        parser.error('save profiling requires quiet replay timing')
    if args.profile_save_detail and not args.profile_save:parser.error('detailed profiling requires --profile-save')
    if args.benchmark:args.self_restore=True
    if args.correction:args.resimulate=True
    if args.resimulate:args.rewind=True
    if args.rewind:args.self_restore=True
    if args.history and not args.resimulate:parser.error('history requires --resimulate or --correction')
    if args.replay_timing and not (args.resimulate and args.history and args.before_input):
        parser.error('replay timing requires resimulation, history and pre-input capture')
    if args.quiet_replay_timing and (not args.replay_timing or args.correction or args.benchmark):
        parser.error('quiet timing requires --replay-timing and same-input replay without warm microbenchmarks')
    if args.before_input and not args.history:parser.error('pre-input phase requires --history')
    if args.rounds>1 and (args.correction or not args.rewind):parser.error('multiple intervals currently require same-input rewind')
    if args.target<0 or args.target>1000000:parser.error('target must be 0..1000000')
    if args.history and args.target<120 and not args.before_input:parser.error('early history targets require --before-input')
    if args.rewind and args.samples<=args.target+120*(args.rounds-1)+args.depth:parser.error('replay must extend beyond the last requested history interval')
    out = args.output.resolve()
    if out.exists() or args.samples < 360:
        parser.error('Use a fresh output directory and at least 360 samples')
    family_receipt = args.family_receipt.resolve()
    prior = json.loads(family_receipt.read_text())
    family = next(case for case in prior['cases'] if case['family'] == 'iceclimbers')
    if prior['status'] != 'PASS' or family['status'] != 'PASS':
        parser.error('Need a previously accepted source family receipt')
    out.mkdir(parents=True)
    store = out / 'content'
    shutil.copytree(family_receipt.parent / 'iceclimbers/store', store)
    exe = ROOT / 'build/native/melee_native.exe'
    iso = ROOT.parent / 'Super Smash Bros. Melee (USA) (v1.02).iso'
    replay = out / 'inputs.fmr'
    replay.write_bytes(struct.pack('<8sQII', b'FMREPLAY', 0, 0x12345678, 1) +
                       bytes(80) * args.samples)
    corrected = out / 'corrected.fmr'
    if args.correction:
        data = bytearray(replay.read_bytes())
        for sample in range(args.target, args.target+args.depth):
            struct.pack_into('<Hbb', data, 24 + 80 * sample + 8,
                             0x400 if sample == args.target else 0, 80, 0)
        corrected.write_bytes(data)
    comparison = ROOT / 'tools/compare_fighter_parity.py'
    spec = importlib.util.spec_from_file_location('comparison', comparison)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    pins = [Path(__file__).resolve(), Path(__file__).parent/'trace_interval.py', exe, iso, replay, comparison, family_receipt]
    if args.correction:pins.append(corrected)
    pins += [p for p in store.rglob('*') if p.is_file()]
    pins += [ROOT/'src/melee/ef'/n for n in ['efdata.c','eflib.c','eflib.h','native_effect_control_snapshot.c','native_effect_control_snapshot.h']]
    pins += [ROOT/'src/sysdolphin/baselib'/n for n in ['particle.c','generator.c','psappsrt.c','psdisp.c','psdisp_state.c','psdisp_state_internal.h','particle_state.c','particle_state_internal.h','native_particle_control_snapshot.c','native_particle_control_snapshot.h']]
    pins += [ROOT/'src/sysdolphin/baselib'/n for n in ['audio_rollback.c','audio_rollback.h','axdriver.c']]
    pins += [ROOT/'src/sysdolphin/baselib'/n for n in ['lobj.c','lobj.h','gobj.c','lobj_state.c','lobj_state_internal.h','native_light_control_snapshot.c','native_light_control_snapshot.h']]
    pins += [ROOT/'src/melee/lb'/n for n in ['lb_00F9.c','lb_020A.c']]
    pins += [ROOT/'src/melee/ft/ftdata.c',ROOT/'src/melee/ft/ftdata.h',ROOT/'src/melee/it/item.c']
    pins += [ROOT/'src/melee/ft'/n for n in ['native_script_fighter_actor.c','native_script_fighter.h','native_script_fighter_snapshot.h','native_script_execution.c','native_callback_lifetime.c']]
    pins += [ROOT/'src/melee/gm/native_match_history.c',ROOT/'src/melee/gm/native_match_history.h']
    pins += [ROOT/'src/melee/gm/gm_1A45.c']
    pins += [ROOT/'src/melee/gr'/name for name in ['ground.c','stage_start_state.c','stage_start_state.h','native_stage_start_snapshot.c','native_stage_start_snapshot.h']]
    pins += [ROOT/'src/melee/if'/name for name in ['ifall.c','ifall_state.c','ifall_state_internal.h','if_2F6E.c','if_2F72.c','ifnametag.c','ifnametag_state.c','ifnametag_state_internal.h','textdraw.c','textdraw.h']]
    pins += [ROOT.parent/'aurora/lib/gfx/recording.cpp']
    pins += [ROOT/'src/melee/gm'/n for n in ['native_replay.c','native_replay.h','native_replay_state.c']]
    pins += sorted((ROOT / 'src/melee/gm').glob('native_snapshot_census.*'))
    pins += [ROOT/'src/melee/gm'/n for n in ['native_match_snapshot.h','native_match_snapshot_capture.c','native_match_snapshot_load.c']]
    pins += sorted((ROOT / 'src/melee/gm').glob('native_snapshot_actor_graph.*'))
    pins += sorted((ROOT / 'src/melee/gm').glob('native_snapshot_class_graph.*'))
    pins += sorted((ROOT / 'src/melee/gm').glob('native_snapshot_endpoint.*'))
    pins += [ROOT/'src/melee/gm'/n for n in ['native_texture_identity_snapshot.c','native_texture_identity_snapshot.h']]
    pins += [ROOT.parent/'aurora/lib/dolphin/gx/GXTexture.cpp']
    pins += [ROOT/n for n in ['src/melee/gm/gmvs.c','src/melee/gm/gmvs_state.c','src/melee/gm/gmvs_state_internal.h','src/melee/gm/native_versus_control_snapshot.c','src/melee/gm/native_versus_control_snapshot.h','src/melee/it/itspawn.c','src/melee/it/itspawn_state.c','src/melee/it/itspawn_state_internal.h','src/melee/if/ifstatus.c','src/melee/if/ifstatus_state.c','src/melee/if/ifstatus_state_internal.h']]
    pins += sorted((ROOT / 'src/melee/gm').glob('native_rollback_regions.*'))
    pins += [ROOT / 'CMakeLists.txt', ROOT / 'src/melee/gm/native_match_launch.c',
             ROOT / 'src/melee/gm/gmmain.c', ROOT / 'src/melee/gm/gm_1A45.c',
             ROOT / 'src/melee/gm/gm_1A3F.c']
    pins += [ROOT / 'src/sysdolphin/baselib' / name for name in
             ('memory.c', 'robj.c', 'native_memory_snapshot.h', 'native_robj_snapshot.h')]
    pins += [ROOT / 'src/sysdolphin/baselib' / name for name in
             ('controller.c', 'rumble.c', 'native_controller_snapshot.c',
              'native_controller_snapshot.h', 'native_controller_snapshot_internal.h')]
    pins += [ROOT / 'src/sysdolphin/baselib' / name for name in
             ('random.c', 'native_random_snapshot.h')]
    pins += [ROOT / 'src/melee/lb' / name for name in
              ('lb_0195.c', 'native_input_clock_snapshot.h', 'lbvector.c')]
    pins += [ROOT / 'src/melee/gm' / name for name in
             ('gm_1A45.static.h', 'native_gm_loop_snapshot.h', 'gm_1A36.c',
              'gm_1A36.static.h', 'native_gm_input_snapshot.h', 'gm_1601.c', 'gm_1601.h')]
    pins += [ROOT / 'src/melee/gm' / name for name in ('gm_16A2.c', 'gm_16A2.h')]
    pins += [ROOT.parent / 'aurora/lib/dolphin/os/OSAlloc.cpp',
             ROOT.parent / 'aurora/include/dolphin/os/OSHeapSnapshot.h',
             ROOT / 'extern/dolphin/include/dolphin/os/OSHeapSnapshot.h']
    pins += [ROOT / 'src/melee/pl' / name for name in
             ('player.c', 'player_state.c', 'player.h', 'player_internal.h', 'types.h', 'plstale.c',
              'plattack.c', 'native_player_snapshot.c', 'native_player_snapshot.h')]
    pins += [ROOT / 'src/melee/cm' / name for name in
             ('camera.c', 'camera_subjects.c', 'camera_subjects_internal.h',
              'native_camera_subject_snapshot.h', 'types.h', 'forward.h')]
    pins += [ROOT / 'src/melee/cm' / name for name in
             ('camera_state.c', 'camera_state_internal.h', 'native_camera_snapshot.c',
              'native_camera_snapshot.h')]
    pins += [ROOT / 'src/sysdolphin/baselib' / name for name in
             ('class.c', 'native_class_snapshot.h')]
    pins += [ROOT / 'src/melee/gr' / name for name in
             ('stage_state.c', 'stage_state_internal.h', 'native_stage_snapshot.c',
              'native_stage_snapshot.h', 'ground.c', 'grlib.c', 'stage.c', 'stage.h', 'types.h')]
    pins += [ROOT / 'src/melee/gr' / name for name in
             ('stage_hazard_state.c','stage_hazard_state_internal.h','grrcruise.c','grrcruise_data.h','grizumi.c','native_stage_hazard_snapshot.c',
              'native_stage_hazard_snapshot.h','grcorneria.c','grvenom.c','grgreens.c','grkraid.c','grzebes.c','grkongo.c','grkongo.static.h','gricemt.c','gricemt_data.h')]
    pins += [ROOT / 'src/melee/gr' / name for name in
             ('native_green_grid.c','native_green_grid_snapshot.c','native_green_grid.h')]
    pins += [ROOT/'src/melee/lb'/name for name in ('lb_00F9.c','dynamics_state.c','dynamics_state_internal.h','native_dynamics_snapshot.c','native_dynamics_snapshot.h','types.h')]
    pins += [ROOT/'src/melee/ft/ftdynamics.c']
    pins += [ROOT/'src/melee/lb/lb_00F9.h']
    pins += [ROOT/'src/melee/gr/grpura.c',ROOT/'src/melee/gr/grbattle.c']
    pins += [ROOT/'src/melee/gr/grbigblue.c',ROOT/'src/melee/gr/grbigblue.static.h']
    pins += [ROOT/'src/melee/gr'/n for n in ['grzakogenerator.c','grzakogenerator.h','stage_generator_state.c','stage_generator_state.h','native_stage_generator_snapshot.c','native_stage_generator_snapshot.h','stage_spawn_state.c','stage_spawn_state.h','native_stage_spawn_snapshot.c','native_stage_spawn_snapshot.h']]
    pins += [ROOT/'src/melee/gm/gmregtyfall.c']
    pins += [ROOT/'src/melee/sfx'/name for name in ['crowdsfx.c','crowdsfx.h','sfx_unk.c']]
    pins += [ROOT/'src/sysdolphin/baselib'/name for name in ['mobj.c','mobj.h','tobj.c','tobj.h','native_material_texture_snapshot.c','native_material_texture_snapshot.h']]
    pins += [ROOT/'src/melee/lb/lbshadow.c']
    pins += sorted((ROOT/'src/melee/vi').glob('*.c'))
    # Pin the full Melee source/header tree, including native pool writers and
    # their shared metadata contracts, rather than a partial list.
    pins += sorted(p for p in (ROOT/'src').rglob('*') if p.suffix in {'.c','.cpp','.h','.hpp','.inl'})
    pins += sorted(p for p in (ROOT.parent/'aurora/lib').rglob('*') if p.suffix in {'.cpp','.hpp','.h'})
    pins += sorted(p for p in (ROOT.parent/'aurora/include').rglob('*') if p.suffix in {'.hpp','.h'})
    pins = list(dict.fromkeys(pins))
    receipt = dict(status='RUNNING', skip_gpu_output=args.skip_gpu_output, skip_model_geometry=args.skip_model_geometry,profile_save=args.profile_save,profile_save_detail=args.profile_save_detail,quiet_replay_timing=args.quiet_replay_timing,replay_timing=args.replay_timing,benchmark=args.benchmark,stage=args.stage,samples=args.samples,self_restore=args.self_restore,rewind=args.rewind,correction=args.correction,history=args.history,before_input=args.before_input,depth=args.depth,rounds=args.rounds,target=args.target,family=family['package'],
                   input_sha256={str(p): sha(p) for p in pins}, cases=[], limits=[
                       'Optional bounded same/corrected-input rewind; no general production history or complete snapshot closure.',
                       'Offline diagnostic timing includes current readiness/history checks; not a production deadline guarantee.',
                       'Previously accepted source family reused and pinned; not fresh fighter translation.',
                       'Normal HSD draw retained; hidden/muted fast pacing.',
                       'Owner rejection is recorded as incomplete coverage, not suppressed.'])
    if args.quiet_replay_timing:
        receipt['limits'].append('Endpoint-only timing cohort: no independent forward trace or intermediate per-frame comparison. Ordinary traced correctness runs remain necessary.')
    target = out / 'receipt.json'

    def save():
        target.write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')

    save()
    for name, scripted in [('stock-eight', False), ('interpreted-eight', True)]:
        case = dict(name=name, runs=[], differences=[], owners=[])
        traces = {}
        labels = ('census',) if args.quiet_replay_timing else (
            ('reference', 'corrected-reference', 'census') if args.correction else ('reference', 'census'))
        for label in labels:
            log = out / f'{name}-{label}.log'
            diagnostic = out / f'{name}-{label}-owners.log'
            command = [str(exe), '--iso', str(iso), '--match', '--stage', args.stage,
                       '--mode', 'stock', '--stocks', '99', '--time', '0', '--items', 'medium',
                       '--hidden', '--mute', '--fast',
                       '--replay-inputs', str(corrected if label=='corrected-reference' else replay), '--content-root', str(store)]
            if not args.quiet_replay_timing:
                command += ['--object-census', '--trace-fighter-state', '--trace-actions']
            for port in range(1, 5):
                slot = ('human:custom' if scripted else 'human:iceclimbers') if args.correction and port==1 else ('cpu:custom:9' if scripted else 'cpu:iceclimbers:9')
                command += [f'--p{port}', slot]
                if scripted:
                    command += [f'--p{port}-fighter', family['package']]
            if label == 'census':
                command.append('--snapshot-census')
            env = os.environ.copy()
            env['FREAK_MELEE_SKIP_MODEL_RESIM']='1' if args.skip_model_geometry else '0'
            env['FREAK_MELEE_NO_GPU_RESIM']='1' if args.skip_gpu_output else '0'
            env['FREAK_MELEE_SNAPSHOT_CENSUS'] = str(diagnostic)
            env['FREAK_MELEE_SNAPSHOT_SELF_RESTORE']='1' if args.self_restore and label=='census' else '0'
            env['FREAK_MELEE_COMPACT_SNAPSHOT_BENCHMARK']='1' if args.benchmark and label=='census' else '0'
            env['FREAK_MELEE_COMPACT_REPLAY_TIMING']='1' if args.replay_timing and label=='census' else '0'
            env['FREAK_MELEE_PROFILE_MATCH_SAVE']='1' if args.profile_save and label=='census' else '0'
            env['FREAK_MELEE_PROFILE_MATCH_SAVE_DETAIL']='1' if args.profile_save_detail and label=='census' else '0'
            env['FREAK_MELEE_SNAPSHOT_REWIND']='1' if args.rewind and label=='census' else '0'
            env['FREAK_MELEE_SNAPSHOT_RESIMULATE']='1' if args.resimulate and label=='census' else '0'
            env['FREAK_MELEE_SNAPSHOT_RESIMULATION_RAW']=str(out/f'{name}-resim-raw.log')
            env['FREAK_MELEE_SNAPSHOT_REWIND_DEPTH']=str(args.depth)
            env['FREAK_MELEE_SNAPSHOT_REWIND_ROUNDS']=str(args.rounds)
            env['FREAK_MELEE_SNAPSHOT_REWIND_TARGET']=str(args.target)
            env['FREAK_MELEE_SNAPSHOT_HISTORY']='1' if args.history and label=='census' else '0'
            env['FREAK_MELEE_SNAPSHOT_BEFORE_INPUT']='1' if args.before_input and label=='census' else '0'
            env.pop('FREAK_MELEE_SNAPSHOT_CORRECTED_INPUTS', None)
            if args.correction and label=='census':env['FREAK_MELEE_SNAPSHOT_CORRECTED_INPUTS']=str(corrected)
            env['FREAK_MELEE_OBJECT_CENSUS'] = str(out / f'{name}-{label}-objects.log')
            errors = []
            with log.open('wb') as stream:
                try:
                    result = subprocess.run(command, cwd=ROOT, env=env, stdout=stream,
                                            stderr=subprocess.STDOUT, timeout=240,
                                            creationflags=subprocess.CREATE_NO_WINDOW)
                    code = result.returncode
                except subprocess.TimeoutExpired:
                    code = None
                    errors.append('timeout; child killed')
            text = log.read_text(errors='replace')
            if code != 0:
                errors.append(f'exit={code}')
            if f'[replay] End after {args.samples} controller samples.' not in text:
                errors.append('incomplete replay')
            if re.search(r'^\[script\].*faulted=[1-9]|\[snapshot-census\] error:', text, re.M):
                errors.append('VM or census failure')
            trace_log=log
            if args.resimulate and label=='census' and not args.quiet_replay_timing:
                if args.correction:
                    trace_log,interval_errors=compare_correction(module,log,out/f'{name}-corrected-reference.log',out,name,args.depth,args.target)
                else:trace_log,interval_errors=compare_interval(module,log,out,name,args.depth,args.rounds,args.target)
                errors+=interval_errors
            if args.quiet_replay_timing:
                if re.search(r'^\[(?:fighter|companion|action|rng)\]', text, re.M):
                    errors.append('quiet timing unexpectedly emitted per-frame traces')
                if (out / f'{name}-{label}-objects.log').exists():
                    errors.append('quiet timing unexpectedly enabled object census')
                markers = re.findall(r'^\[snapshot-census\] resimulation-(reference-start|reference-end|trace-start|trace-end)$', text, re.M)
                if markers != ['reference-start', 'reference-end', 'trace-start', 'trace-end'] * args.rounds:
                    errors.append('quiet timing replay interval boundaries absent or out of order')
            else:
                trace, end, _ = module.read_trace(trace_log, {1, 2, 3, 4}, set())
                if end != args.samples or not trace['fighter'] or not trace['companion']:
                    errors.append('eight-actor traces absent or incomplete')
                traces[label] = trace
            if label == 'census':
                diag = diagnostic.read_text() if diagnostic.exists() else ''
                if args.self_restore:
                    arena_rows=re.findall(r'^\[snapshot-census\] arena-construction span=(hsd|scene) prefix_bytes=(\d+) copied_bytes=(\d+) roots=(\d+) samples=32 save_us=([\d.]+) load_us=([\d.]+) scope=warm-single-span$',diag,re.M)
                    if len(arena_rows)!=2 or {row[0] for row in arena_rows}!={'hsd','scene'}:
                        errors.append('construction-time arena admission/copy probes missing')
                    for span,prefix,copied,roots,save_us,load_us in arena_rows:
                        if int(prefix)<=64 or int(copied)<int(prefix) or not int(roots):errors.append('invalid admitted arena span')
                        case[span+'_arena_probe']=dict(prefix_bytes=int(prefix),copied_bytes=int(copied),roots=int(roots),
                            save_us=float(save_us),load_us=float(load_us),scope='warm-single-span',full_match=False)
                    vm_copy=re.findall(r'^\[snapshot-census\] fighter-vm-copy actors=(\d+) state_bytes=(\d+) match=1$',diag,re.M)
                    expected_actors=8 if name=='interpreted-eight' else 0
                    if len(vm_copy)!=1 or int(vm_copy[0][0])!=expected_actors or (expected_actors and int(vm_copy[0][1])==0):
                        errors.append('fighter VM variables/binding copy probe missing or incomplete')
                    else:case['fighter_vm_copy']=dict(actors=int(vm_copy[0][0]),state_bytes=int(vm_copy[0][1]),match=True)
                if args.resimulate:
                    if args.correction:
                        repeated=re.search(r'^\[snapshot-census\] correction-repeat reset=(\d+) replay_rewound=(\d+) changed_owners=(\d+)$',diag,re.M)
                        if not repeated or repeated.group(1)!='1' or repeated.group(2)!='1' or int(repeated.group(3))==0:errors.append('corrected endpoint did not change or repeat reset failed')
                        case['correction_repeat']=repeated.groups() if repeated else []
                    comparisons=re.findall(r'^\[snapshot-census\] resim-compare owner=(\S+) match=(\d+) bytes=(\d+) first_difference=(\d+)$',diag,re.M)
                    summaries=re.findall(r'^\[snapshot-census\] resim-end ticks=(\d+) compared=(\d+) expected=(\d+) endpoint_restored=(\d+)$',diag,re.M)
                    if len(comparisons)!=args.rounds*len(OWNERS) or {row[0] for row in comparisons}!=OWNERS or any(row[1]!='1' for row in comparisons):
                        errors.append('compact resimulation owner mismatch')
                    if len(summaries)!=args.rounds or any(tuple(map(int,row))!=(args.depth,len(OWNERS),len(OWNERS),1) for row in summaries):errors.append('compact resimulation endpoint recovery incomplete')
                    if diag.count('[snapshot-census] resim-null-load restored=1 replay_rewound=1')!=args.rounds:errors.append('null-report compact reset or replay rewind absent')
                    audio=re.findall(r'^\[snapshot-census\] resim-audio reused=(\d+) new=(\d+) canceled=(\d+)$',diag,re.M)
                    if len(audio)!=args.rounds or any(int(value) for row in audio for value in row[1:]):errors.append('same-input resimulation changed audio starts')
                    raw_end=re.findall(r'^\[snapshot-census\] resim-raw differing_bytes=(\d+)$',diag,re.M)
                    if len(raw_end)!=args.rounds or any(int(value) for value in raw_end):errors.append('resimulation broad endpoint differs')
                    case['resimulation']=dict(compared=len(comparisons),intervals=summaries,owners=comparisons,raw_differing_bytes=list(map(int,raw_end)),audio=[list(map(int,row)) for row in audio])
                counts = re.findall(r'\bfighters=(\d+)', diag)
                scripted_counts = re.findall(r'\bscripted_fighters=(\d+)', diag)
                if not counts or any(int(n) != 8 for n in counts):
                    errors.append('eight live native actors not observed')
                if not scripted_counts or any(int(n) != (8 if scripted else 0) for n in scripted_counts):
                    errors.append('scripted actor count does not match scenario')
                summaries = re.findall(r'^\[snapshot-census\] summary owner=(\S+) (.+)$', diag, re.M)
                if args.replay_timing:
                    rows=re.findall(r'^\[snapshot-census\] replay-timing (.+)$',diag,re.M)
                    case['replay_timing']=[dict(re.findall(r'(\w+)=([\w.]+)',row)) for row in rows]
                    if args.profile_save:
                        profiles=re.findall(r'^\[save-profile\] summary (.+)$',diag,re.M)
                        experiments=re.findall(r'^\[save-profile\] experiment mode=(live-gather|packed-chunks|packed-single) (.+)$',diag,re.M)
                        case['save_profiles']=[dict(re.findall(r'(\w+)=([\w.]+)',row)) for row in profiles]
                        case['copy_experiments']=[dict(mode=mode,**dict(re.findall(r'(\w+)=([\w.]+)',row))) for mode,row in experiments]
                        if len(profiles)!=args.rounds or any(int(row.get('frames',0))!=args.depth for row in case['save_profiles']):
                            errors.append('save profile frame coverage incomplete')
                        if len(experiments)!=3*args.rounds or any(row.get('exact')!='1' for row in case['copy_experiments']):
                            errors.append('save profile copy experiments incomplete or altered data')
                    expected=args.rounds*(2 if args.correction else 1)
                    if len(rows)!=expected or any(any(int(row.get(key,0))!=args.depth for key in
                        ['ticks','history_calls','input_calls','simulation_calls','drawing_calls',
                         'render_begin_calls','hsd_draw_calls','render_end_calls']) or
                        row.get('instrumentation')!=('endpoint_only' if args.quiet_replay_timing else 'retained') for row in case['replay_timing']):
                        errors.append('complete replay timing phase coverage absent')
                if {owner for owner, _ in summaries} != OWNERS:
                    errors.append('owner batch incomplete')
                case['owners'] = [dict(owner=owner, **dict(re.findall(r'(\w+)=([\w.]+)', fields)))
                                  for owner, fields in summaries]
                if args.benchmark:
                    rows=re.findall(r'^\[snapshot-census\] compact-benchmark (.+)$',diag,re.M)
                    case['compact_benchmark']=[dict(re.findall(r'(\w+)=([\w.]+)',row)) for row in rows]
                    if len(rows)!=args.samples//120 or any(int(row.get('exact',0))!=1 or
                        int(row.get('iterations',0))!=16 or int(row.get('bytes',0))<=0
                        for row in case['compact_benchmark']):errors.append('compact benchmark coverage or recapture proof incomplete')
                    if [int(row['sample']) for row in case['compact_benchmark']]!=list(range(120,args.samples+1,120)):
                        errors.append('compact benchmark checkpoint IDs incomplete')
                for owner in case['owners']:
                    if any(int(owner.get(k, 0)) != args.samples // 120 for k in ('attempts', 'saved', 'validated')):
                        errors.append('incomplete owner coverage: ' + owner['owner'])
                if '[snapshot-census] aggregate-buffer-tests refused=1 metadata_unchanged=1' not in diag:
                    errors.append('shared aggregate preflight refusal proof absent')
                if (args.skip_model_geometry or args.skip_gpu_output) and label=='census':
                    text=log.read_text(errors='replace')
                    skipped=re.search(r'\[native-render\] Skipped model primitives: (\d+)\.',text)
                    if not skipped or not int(skipped[1]):errors.append('model geometry omission was not exercised')
                    else:case['model_primitives_skipped']=int(skipped[1])
                    if args.skip_gpu_output:
                        frames=re.search(r'\[native-render\] CPU-only GX frames: (\d+)\.',text)
                        expected_frames=args.rounds*args.depth*(2 if args.correction else 1)
                        if not frames or int(frames[1])!=expected_frames:errors.append('CPU-only GX frame coverage incomplete')
                        else:case['cpu_only_gx_frames']=int(frames[1])
                if args.self_restore:
                    loaded=re.findall(r'^\[snapshot-census\] self-restore-load owner=(\S+) restored=(\d+)(?: elapsed_us=[\d.]+)?$',diag,re.M)
                    compared=re.findall(r'^\[snapshot-census\] self-restore-compare owner=(\S+) match=(\d+) bytes=(\d+)$',diag,re.M)
                    expected=args.samples//120*len(OWNERS)
                    if len(loaded)!=expected+(args.rounds*len(OWNERS) if args.rewind and not args.history else 0) or {n for n,_ in loaded}!=OWNERS or any(v!='1' for _,v in loaded):
                        errors.append('incomplete same-boundary Load coverage')
                    if len(compared)!=expected or {n for n,_,_ in compared}!=OWNERS or any(v!='1' for _,v,_ in compared):
                        errors.append('incomplete same-boundary recapture equality')
                    summary=re.search(r'^\[snapshot-census\] self-restore-summary restored=(\d+) compared=(\d+) expected=(\d+)$',diag,re.M)
                    if not summary or any(int(v)!=expected for v in summary.groups()):
                        errors.append('self-restore summary incomplete')
                    batches=re.findall(r'^\[snapshot-census\] self-restore-boundary owners=(\d+) bytes=(\d+)$',diag,re.M)
                    totals=re.findall(r'^\[snapshot-census\] self-restore-load-total owners=(\d+) elapsed_us=([\d.]+)$',diag,re.M)
                    if len(batches)!=args.samples//120 or len(totals)!=len(batches)+(args.rounds if args.rewind and not args.history else 0) or any(int(n)!=len(OWNERS) for n,_ in batches+totals):
                        errors.append('self-restore batch/Load timing coverage incomplete')
                    actor_graphs=re.findall(r'^\[snapshot-census\] actor-graph valid=(\d+) fighters=(\d+) ground=(\d+) items=(\d+)$',diag,re.M)
                    if len(actor_graphs)!=args.samples//120+(args.rounds if args.rewind and not args.history else 0) or any(valid!='1' or fighters!='8' for valid,fighters,_,_ in actor_graphs):
                        errors.append('candidate actor graph validation incomplete')
                    class_graphs=re.findall(r'^\[snapshot-census\] class-graph valid=(\d+) blocks=(\d+) types=(\d+)$',diag,re.M)
                    if len(class_graphs)!=args.samples//120+(args.rounds if args.rewind and not args.history else 0) or any(valid!='1' for valid,_,_ in class_graphs):
                        errors.append('candidate class graph validation incomplete')
                    case['self_restore_checks']=dict(class_graphs=len(class_graphs),actor_graphs=len(actor_graphs),loaded=len(loaded),compared=len(compared),expected=expected,
                        peak_bytes=max((int(n) for _,n in batches),default=0),load_us=[float(n) for _,n in totals])
                if args.rewind:
                    if args.history:
                        copies=re.findall(r'^\[snapshot-census\] history-copy-load restored=(\d+) spans=(\d+)$',diag,re.M)
                        if copies!=[('1','2')]*args.rounds:errors.append('copy history load coverage incomplete')
                        bound=re.search(r'^\[snapshot-census\] history-copy-bound spans=2 reserved_bytes=(\d+) slot_bytes=(\d+)$',diag,re.M)
                        if not bound or int(bound.group(1))!=16*int(bound.group(2)):errors.append('copy history binding coverage incomplete')
                        loads=re.findall(r'^\[snapshot-census\] history-load target=(\d+) future_invalidated=(\d+) expected=(\d+) stale_refused=(\d+)$',diag,re.M)
                        expected=[(args.target+120*i,args.depth,args.depth,1) for i in range(args.rounds)]
                        if [tuple(map(int,row)) for row in loads]!=expected:errors.append('history future/stale invalidation incomplete')
                        storage=re.search(r'^\[snapshot-census\] history-storage captures=(\d+) restores=(\d+) peak_bytes=(\d+) slots=(\d+) slot_bytes=(\d+) failed=(\d+)$',diag,re.M)
                        captures=args.samples-(0 if args.before_input else 120)+1+args.rounds*args.depth*(2 if args.correction else 1)+(1 if args.correction else 0)
                        restores=args.rounds*(3 if args.correction else 2)
                        if not storage or tuple(map(int,storage.groups()))!=(captures,restores,int(storage.group(3)),16,int(storage.group(5)),0) or int(storage.group(3))<=0 or not bound or int(storage.group(5))!=int(bound.group(2)) or int(storage.group(3))>int(storage.group(5)):errors.append('history capture/load/publication coverage incomplete')
                        case['history_storage']=list(map(int,storage.groups())) if storage else []
                    rewind_summary=re.search(r'^\[snapshot-census\] rewind-summary loaded=(\d+) compared=(\d+) endpoint_restores=(\d+) pending=(\d+)$',diag,re.M)
                    if not rewind_summary or tuple(map(int,rewind_summary.groups()))!=(args.rounds*(2 if args.history else len(OWNERS)),args.rounds*len(OWNERS),args.rounds,0):
                        errors.append('older component restore/recapture or endpoint recovery incomplete')
                    history=re.search(r'^\[snapshot-census\] history-summary rounds=(\d+) expected=(\d+) depth=(\d+)$',diag,re.M)
                    if not history or tuple(map(int,history.groups()))!=(args.rounds,args.rounds,args.depth):errors.append('requested history intervals incomplete')
                    comparisons=re.findall(r'^\[snapshot-census\] rewind-compare owner=(\S+) match=(\d+) bytes=(\d+)$',diag,re.M)
                    if len(comparisons)!=args.rounds*len(OWNERS) or {n for n,_,_ in comparisons}!=OWNERS or any(v!='1' for _,v,_ in comparisons):
                        errors.append('older candidate recapture mismatch')
                    changed=re.findall(r'^\[snapshot-census\] rewind-delta owner=(\S+) changed=1$',diag,re.M)
                    if 'fighter' not in changed or 'rng' not in changed:errors.append('rewind did not exercise advanced fighter/RNG state')
                    raw=re.findall(r'^\[snapshot-census\] rewind-raw-summary differing_bytes=(\d+)$',diag,re.M)
                    if len(raw)!=args.rounds:errors.append('target oracle coverage comparison absent')
                    activity=re.findall(r'^\[snapshot-census\] rewind-object-activity created=(\d+) destroyed=(\d+) reused=(\d+)$',diag,re.M)
                    if len(activity)!=args.rounds:errors.append('rewind object activity absent')
                    coverage=re.findall(r'^\[snapshot-census\] rewind-coverage-summary total=(\d+) pool_state=(\d+) inactive_pool=(\d+) unclassified=(\d+)$',diag,re.M)
                    if len(coverage)!=args.rounds:errors.append('pool coverage accounting absent')
                    for value,row in zip(raw,coverage):
                        total,required,inactive,unknown=map(int,row)
                        if total!=int(value) or total!=required+inactive+unknown:
                            errors.append('pool coverage accounting inconsistent')
                        if required:errors.append('restored pool state differs from target oracle')
                    pool_rows=re.findall(r'^\[snapshot-census\] rewind-pool-coverage owner=(\S+) state=(\d+) free_payload=(\d+) stride_padding=(\d+) free_tail=(\d+)$',diag,re.M)
                    if len(pool_rows)!=8*args.rounds or {row[0] for row in pool_rows}!={'gobj','gobj-proc','fighter','ground','item','item-link','aobj','fobj'}:
                        errors.append('pool coverage owner set incomplete')
                    case['pool_coverage']=dict(intervals=[list(map(int,row)) for row in coverage],owners=pool_rows)
                    case['rewind_checks']=dict(compared=len(comparisons),changed_owners=changed,summary=rewind_summary.groups() if rewind_summary else [],raw_differing_bytes=list(map(int,raw)),object_activity=[dict(zip(['created','destroyed','reused'],map(int,row))) for row in activity])
                case['diagnostic_sha256'] = sha(diagnostic) if diagnostic.exists() else None
            case['runs'].append(dict(command=command, exit=code, errors=errors,
                                     log=str(log), log_sha256=sha(log)))
            save()
        case['trace_comparison_performed'] = not args.quiet_replay_timing
        case['differences'] = None if args.quiet_replay_timing else [kind for kind in traces['reference']
                               if traces['reference'][kind] != traces['census'][kind]]
        case['status'] = 'PASS' if not case['differences'] and all(not r['errors'] for r in case['runs']) else 'FAIL'
        receipt['cases'].append(case)
        save()
        print(json.dumps(dict(case=name, status=case['status'], differences=case['differences'],
                              errors=[r['errors'] for r in case['runs']])), flush=True)
    receipt['unchanged_inputs'] = all(sha(Path(p)) == value for p, value in receipt['input_sha256'].items())
    receipt['status'] = 'PASS' if receipt['unchanged_inputs'] and all(c['status'] == 'PASS' for c in receipt['cases']) else 'FAIL'
    save()
    print(json.dumps(dict(status=receipt['status'], receipt=str(target))), flush=True)
    raise SystemExit(receipt['status'] != 'PASS')


if __name__ == '__main__':
    main()
