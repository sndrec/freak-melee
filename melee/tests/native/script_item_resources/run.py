"""Private frozen-source connected resource collection cohort; no shared build/game."""
import argparse
import importlib.util
import hashlib
import json
import re
import shutil
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def body(source, name):
    match = re.search(r'^[^\n;{}]*\b' + re.escape(name) + r'\s*\([^;{}]*\)\s*\{', source, re.M)
    if not match:
        raise RuntimeError('Missing native comparison body: ' + name)
    end = source.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end]


def samus_native_comparison(source_dir, output_dir):
    """Original Samus routines with the same explicit geometry leaf as the VM."""
    samus = source_dir / 'melee/it/kinds/itsamusgrapple.c'
    hookshot = source_dir / 'melee/it/kinds/itlinkhookshot.c'
    # Keep quoted local includes in their original directory. Both it/ and
    # it/kinds/ have an inlines.h; moving this TU changes which one is selected.
    controlled = samus.parent / 'samus_native_controlled.c'
    controlled.write_text('#define it_802A3D90 fixtureGrappleSweep\n'
                          '#define it_802A4454 fixtureMoveGrappleLine\n'
                          '#define mpCheckAllRemap fixtureSegmentBlocked\n' + samus.read_text())
    sweep = output_dir / 'samus_sweep_actual.h'
    sweep.write_text('#define it_802A3D90 fixtureGrappleSweep\n'
                     '#define mpColl_800471F8 fixtureSweepStage\n'
                     + body(hookshot.read_text(), 'it_802A3D90')
                     + '\n#undef mpColl_800471F8\n#undef it_802A3D90\n')
    line = output_dir / 'samus_line_actual.h'
    line.write_text('#define it_802A4454 fixtureMoveGrappleLine\n'
                    '#define mpLib_80054ED8 fixtureLineEnabled\n'
                    '#define mpGetSpeed fixtureLineSpeed\n'
                    + body(hookshot.read_text(), 'it_802A4454')
                    + '\n#undef mpGetSpeed\n#undef mpLib_80054ED8\n#undef it_802A4454\n')
    stage = source_dir / 'melee/ft/native_script_binding_stage.c'
    controlled_stage = stage.parent / 'stage_controlled.c'
    controlled_stage.write_text('#define mpCheckAllRemap fixtureSegmentBlocked\n'
                               '#define mpLib_80054ED8 fixtureLineEnabled\n'
                               '#define mpLib_8004D164 fixtureGetMap\n'
                               '#define mpGetGroundCollLine fixtureGetLines\n'
                               '#define mpGetGroundCollVtx fixtureGetVertices\n'
                               + stage.read_text())
    return controlled, sweep, line, controlled_stage


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--compiler', type=Path, required=True)
    project_group=p.add_mutually_exclusive_group(required=True)
    project_group.add_argument('--link', type=Path)
    project_group.add_argument('--samus-project', type=Path, help='Run the full authored Samus resource constructor with retail assets.')
    p.add_argument('--common-items', type=Path, help='Retail ItCo archive for the connected Samus factory/attachment cohort.')
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--core-only', action='store_true', help='Diagnostic run excluding animation prepare/static controls; final acceptance uses default.')
    p.add_argument('--frozen', type=Path,
                   help='Reuse production/library/draft snapshot from an earlier run; current fixture files are copied separately.')
    a = p.parse_args()
    out = a.output.resolve()
    if out.drive.upper() != 'A:' or out.exists():
        p.error('fresh private A: output required')
    build = ROOT / 'build/native'
    base = a.frozen.resolve() if a.frozen else None
    if a.samus_project and (base or a.core_only):
        p.error('Samus requires current production inputs and full animation preparation')
    if a.common_items and not a.samus_project:
        p.error('Common Item assets are currently used by the Samus factory cohort')
    if base:
        prior = json.loads((base / 'freeze.json').read_text())
        if prior['status'] != 'FROZEN' or not all(sha(Path(path)) == value for path, value in prior['snapshot_sha256'].items()):
            p.error('prior frozen snapshot has changed')
        if sha(a.compiler.resolve()) != prior['compiler_sha256']:
            p.error('compiler differs from frozen snapshot')
        build = base / 'snapshot'
    ninja = (build / 'build.ninja').read_text()
    begin = ninja.index('build native_source_assets_test.exe:')
    block = ninja[begin:ninja.index('\n\n', begin)]
    libs = re.search(r'^  LINK_LIBRARIES = (.+)$', block, re.M).group(1).split()
    out.mkdir(parents=True)
    snapshot = out / 'snapshot'
    snapshot.mkdir()
    pairs = []
    for folder, target in [(build / 'src' if base else ROOT / 'src', snapshot / 'src'),
                           (build / 'dolphin' if base else ROOT / 'extern/dolphin/include', snapshot / 'dolphin'),
                           (build / 'aurora' if base else ROOT.parent / 'aurora/include', snapshot / 'aurora')]:
        for source in folder.rglob('*'):
            if source.is_file():
                pairs.append((source, target / source.relative_to(folder)))
    for name in ['test.c', 'run.py', 'README.md', 'samus_constructor_cases.h', 'palette_cases.h', 'samus_factory_cases.h', 'samus_held_cases.h', 'samus_solver_cases.h', 'scheduler_cases.h', 'graph_buffer_cases.h', 'resource_snapshot_cases.h']:
        pairs.append((HERE / name, snapshot / name))
    if a.common_items:
        pairs.append((a.common_items,snapshot/'ItCo.dat'))
    for name in dict.fromkeys(n for n in libs if not n.startswith('-')):
        pairs.append((build / 'lib' / name if base else build / name, snapshot / 'lib' / name))
    for dll in (base if base else build).glob('*.dll'):
        pairs.append((dll, out / dll.name))
    draft = build / 'draft' if base else (a.samus_project or a.link)
    if a.samus_project:
        composition_path=ROOT/'tests/native/samus_grapple_constructor/compose.py'
        spec=importlib.util.spec_from_file_location('samus_composition',composition_path)
        composition=importlib.util.module_from_spec(spec);spec.loader.exec_module(composition)
        for path in [composition_path,*composition.INPUTS]:
            pairs.append((path,snapshot/'authoring'/path.relative_to(ROOT)))
    for source in draft.rglob('*'):
        if source.is_file():
            pairs.append((source, snapshot / 'draft' / source.relative_to(draft)))
    pairs.append((build / 'build.ninja', snapshot / 'build.ninja'))
    original = {str(source.resolve()): sha(source) for source, _ in pairs}
    for source, target in pairs:
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
    copied = {str(target.resolve()): sha(target) for _, target in pairs}
    freeze_ok = all(sha(source) == original[str(source.resolve())] == copied[str(target.resolve())]
                    for source, target in pairs)
    frozen = {'status': 'FROZEN' if freeze_ok else 'FAIL',
              'original_sha256': original, 'snapshot_sha256': copied,
              'compiler_sha256': sha(a.compiler.resolve()),
              'production_parent_freeze': str(base / 'freeze.json') if base else None,
              'production_parent_freeze_sha256': sha(base / 'freeze.json') if base else None,
              'scope': 'Subsequent compilation uses only private copied source/includes/library/draft inputs; later live edits are independent.'}
    (out / 'freeze.json').write_text(json.dumps(frozen, indent=2) + '\n')
    print(json.dumps({'freeze': str(out / 'freeze.json'), 'status': frozen['status']}), flush=True)
    if not freeze_ok:
        raise SystemExit('live input changed while snapshot copied')
    flags = ['-std=gnu11', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
             '-Wno-unused-function', '-Wno-unused-variable', '-Wno-array-parameter',
             '-fno-zero-initialized-in-bss', '-DMELEE_NATIVE=1', '-D_CRT_SECURE_NO_WARNINGS',
             '-D_USE_MATH_DEFINES', '-D_DLL', '-D_MT', '-Xclang', '--dependent-lib=msvcrt',
             '-fuse-ld=lld', '-Wl,/opt:ref', '-I' + str(snapshot / 'src'),
             '-I' + str(snapshot / 'dolphin'), '-I' + str(snapshot / 'aurora'), '-I' + str(snapshot / 'src/melee/it')]
    resource_source = snapshot / 'src/melee/it/native_article_resource.c'
    controlled = resource_source.read_text()
    substitutions = {'mpColl_800471F8': 'fixtureSweepStage', 'mpColl_80048844': 'fixtureSweepPoint',
                     'mpColl_8004B108': 'fixtureFollowFloor', 'mpGetSpeed': 'fixtureLineSpeed',
                     'mpLib_8004D164': 'fixtureGetMap', 'mpGetGroundCollLine': 'fixtureGetLines',
                     'mpGetGroundCollVtx': 'fixtureGetVertices',
                     'efSync_Spawn': 'fixtureSpawnEffect'}
    # Symbol substitution only at the explicit external leaf boundary. Headers
    # receive the same renames so prototypes remain actual native signatures.
    prefix = ''.join('#define ' + old + ' ' + new + '\n' for old, new in substitutions.items())
    controlled_source = snapshot / 'resource_controlled.c'
    controlled_source.write_text(prefix + controlled)
    copied[str(controlled_source.resolve())] = sha(controlled_source)
    samus_controlled, samus_sweep, samus_line, samus_stage = samus_native_comparison(snapshot / 'src', snapshot)
    for path in [samus_controlled, samus_sweep, samus_line, samus_stage]:
        copied[str(path.resolve())] = sha(path)
    # Unchanged native bodies operate on explicit controlled map tables. Only
    # symbol names change; mpGetSpeed, remapping and its validity predicate are
    # executed, rather than replaced by a recording line-speed leaf.
    native_names = ['mpLib_8004D164', 'mpGetGroundCollLine', 'mpGetGroundCollVtx',
                    'mpRemap2d', 'mpLib_80054ED8', 'mpGetSpeed']
    native_source = (snapshot / 'src/melee/mp/mplib.c').read_text()
    renames = {key: value for key, value in substitutions.items() if key in native_names}
    renames.update({'mpLib_80054ED8': 'fixtureLineEnabled', 'mpLib_804D64B4': 'fixtureMap',
                    'groundCollLine': 'fixtureLines', 'groundCollVtx': 'fixtureVertices',
                    'DbLevel': 'fixtureDebugLevel'})
    native_header = snapshot / 'line_speed_actual.h'
    native_header.write_text(''.join('#define ' + key + ' ' + value + '\n' for key, value in renames.items())
                            + '\n\n'.join(body(native_source, name) for name in native_names)
                            + '\n' + ''.join('#undef ' + key + '\n' for key in renames))
    copied[str(native_header.resolve())] = sha(native_header)
    (out / 'controlled_source.json').write_text(json.dumps({'original_sha256': sha(resource_source), 'generated_sha256': sha(controlled_source), 'substitutions': substitutions}, indent=2) + '\n')
    if a.samus_project:
        frozen_composition=snapshot/'authoring/tests/native/samus_grapple_constructor/compose.py'
        spec=importlib.util.spec_from_file_location('frozen_samus_composition',frozen_composition)
        composition=importlib.util.module_from_spec(spec);spec.loader.exec_module(composition)
        source_path=snapshot/'samus.fms'
        source_path.write_text('fighter:\n    behavior independent\n'+composition.compose(),encoding='utf-8',newline='\n')
        copied[str(source_path.resolve())]=sha(source_path)
    flags += ['-include', str(snapshot / 'src/melee/gm/native_rollback_state_section.h')]
    command = [str(a.compiler.resolve())] + flags + [str(snapshot / 'test.c'), str(controlled_source),
               str(snapshot / 'src/melee/it/native_article_resource_scene.c'),
               str(snapshot / 'src/melee/it/native_article_resource_model.c'),
               str(snapshot / 'src/melee/it/native_article_resource_animation.c'),
               str(snapshot / 'src/melee/it/native_article_resource_snapshot.c')]
    if a.samus_project:
        command += [str(samus_controlled), str(samus_stage)]

    command += [n if n.startswith('-') else str(snapshot / 'lib' / n) for n in libs]
    command += ['-o', str(out / 'test.exe')]
    commands, results = [], []
    stages = [('compile', command), ('fixture', [str(out / 'test.exe'), str(snapshot / 'draft')] + (['core'] if a.core_only else []))]
    if a.samus_project:
        stages=[('compile',command),('samus_constructor',[str(out/'test.exe'),str(snapshot/'draft'),'samus',str(source_path)])]
        if a.common_items:stages[-1][1].append(str(snapshot/'ItCo.dat'))
    elif not a.core_only:
        stages.append(('animated_fixture', [str(out / 'test.exe'), str(snapshot / 'draft'), 'animated']))
    for stage, cmd in stages:
        result = subprocess.run(cmd, cwd=out, capture_output=True, text=True, timeout=120,
                                creationflags=subprocess.CREATE_NO_WINDOW)
        (out / (stage + '.log')).write_text(result.stdout + result.stderr)
        commands.append({'stage': stage, 'command': cmd, 'exit': result.returncode})
        (out / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n')
        results += [json.loads(line) for line in result.stdout.splitlines() if line.startswith('{')]
        if result.returncode:
            break
    unchanged = all(sha(Path(path)) == value for path, value in copied.items())
    unchanged = unchanged and sha(a.compiler.resolve()) == frozen['compiler_sha256']
    live_match = all(sha(Path(path)) == value for path, value in original.items())
    status = 'PASS' if result.returncode == 0 and unchanged and (base or live_match) else 'FAIL'
    constructor_functions = {name: hashlib.sha256(body(controlled, name).encode()).hexdigest()
                             for name in ['nativeArticleResource_AttachmentPhysical',
                                          'nativeArticleResource_CancelConstructingAttachment',
                                          'nativeArticleResource_DetachBorrowers',
                                          'nativeArticleResource_ReleaseCollection']}
    receipt = {'status': status, 'commands': commands, 'results': results,
               'constructor_cleanup_functions_sha256': constructor_functions,
               'animation_mode': 'retail Samus constructor through full prepared candidate; real node/model/controller comparison' if a.samus_project else 'excluded diagnostic cohort' if a.core_only else 'source-derived scene recipes, static Arrow controls and 39 VM/native animated Boomerang graph pairs with recipe/path/bundle refusals',
               'snapshot_sha256': copied, 'snapshot_unchanged': unchanged,
               'live_inputs_match_snapshot': live_match,
               'compiler_sha256': frozen['compiler_sha256'], 'freeze': str(out / 'freeze.json'),
               'scope': 'Actual collection/scene/model/animation kernels, source compiler/Item+Fighter actors/VM, real GObj/Fighter/payload/graph generations and Regions/Pages restore with matrix mutation and deterministic replay; failed-constructor physical attachment cancellation with immediate/deferred imported graph disposal and prior borrower detach; source animation methods compared with original HSD graph/controller/track operations.',
               'controlled': ['private selected candidate/runtime/main-heap inputs, actual owned Link Arrow and Boomerang DAT roots and Boomerang joint bundle',
                              'explicit small Fighter rig and Item heap payload construction/cleanup, actual fixed Fighter pool and actor Attach/VM; retirement flag/life change staged directly for resource cases',
                              'copied resource TU renames stage/point/floor and effect leaves; source-identical mpGetSpeed/remap/validity/getters execute on controlled map geometry',
                              'intentional relation/userdata substitutions and synchronous geometry leaf release; not a stock geometry callback claim'],
               'limits': ['no game, renderer, GPU/audio, production package-loader/Item constructor/whole-match rollback acceptance',
                          'bounded imported Arrow/Boomerang graphs and controlled two-joint attachment rig',
                          'joint animation playback and source Add delivery proved on Boomerang; material/shape resource animation and native Ness gameplay not exercised',
                          'no draw traversal or lazy-matrix allocation exercise; world-matrix assignment/read and captured graph restoration are exercised',
                          'direct unmanaged JObj pointers have no per-address generation; pool graph tokens guard admitted owned graph replacement',
                          'resource effect leaf aborts if reached: no effect allocation acceptance',
                          'native included runtime/heap statics suppress unused-function/variable, SDK array prototype exception retained']}
    if a.samus_project:
        receipt['scope']='Full composed unregistered Samus source prepared privately; actual Created hook/VM, resource kernels and retail HSD construction, held graph/controller/matrix updates and pre-launch hitlag compared with unchanged native routines.'
        receipt['controlled']=['private package publication, manually constructed Item and imported costume rig; full game factory/scheduler not exercised','stage/effect resource leaves controlled and not reached during construction']
        receipt['limits']=['no fighter creation/attach/scale-after-attach orchestration, flight/constraint solver, gameplay or renderer acceptance; held inputs/counters are staged','independence declaration is private fixture-only; Samus remains unregistered','native spawn collision-group undefined memory is explicitly initialized to zero in the script']
        if a.common_items:
            receipt['scope']+=' Real shared Item factory plus interpreted fighter create, bone attachment, owner scaling, bound normal/pre-launch hitlag callbacks and removal execute against imported common Item data. Seventy-two staged deployment cases per mode compare interpreted mode-5 capture and mode-4 ordinary retraction with unchanged native fn_802B8814/fn_802B8684 through completion, including RNG progression, joint/controller/material/palette state and override world matrices. Post-launch hitlag constraints compare with original it_802BACC4. Real CatchPull hooks bind the endpoint and completed source removal releases it; ordinary completion returns to held mode and continues animating.'
            receipt['scope']+=' Another 144 cases compare modes 1/2 extension and falling flight with original fn_802B805C/fn_802B8384 through eight ticks, including ensuing mode-2/3 updates. The original sweep wrapper and resource sweep use the same controlled geometry leaf; contact flags, surface indices/normals and script-owned deployment/floor arrays are compared alongside graph and chain state. Another 216 cases compare modes 3/6/8 with original fn_802B8524/fn_802B895C/fn_802B8D38 through eight ticks or the first transition. Hanging includes owner position/velocity/displacement and timer changes; original line-motion math executes on controlled map tables. Unblocked segment query inputs are compared in order.'
            receipt['scope']+=' Another 72 mode-7 pairs compare nonterminal pull constraints with fn_802B8B54 for two ticks or a ground transition. Each begins with an over-length endpoint gap and verifies its clamp. The native direction-vector expression is corrected to match verified US 1.02 retail stack loads, rather than comparing against the decompiled uninitialized pointer.'
            receipt['controlled']=['private package publication and manually staged parent Fighter payload/rig; the shared Item factory is unchanged','solver entry and chain deployment/positions are staged; no-contact, left-wall and floor responses use the same controlled geometry leaf in both paths','mode 1 AirCatch motion 357 uses floor instead of wall contact; its full Fighter/effect transition remains outside this cohort','modes 6/8 use two unblocked segment-query leaves and original map remapping on controlled moving-line tables; input/ground/timer conditions are staged','effect resource leaves controlled; no scheduler/draw traversal']
            receipt['limits'][0]='no full Fighter creation, actual stage geometry, terminal mode-7 ledge/jump completion or hang expiry/blocked-terrain exits, actual captured victim, gameplay or renderer acceptance; parent payload, capture state and deployment/positions are staged'
            receipt['limits'].append('native retraction comparison graphs are explicitly disposed after completion; source completed removal is real, but native completed-item removal is not compared')
            receipt['native_flight_comparison']={
                'samus_source_sha256':sha(snapshot/'src/melee/it/kinds/itsamusgrapple.c'),
                'samus_controlled_sha256':sha(samus_controlled),
                'stage_controlled_sha256':sha(samus_stage),
                'line_motion_body_sha256':hashlib.sha256(body((snapshot/'src/melee/it/kinds/itlinkhookshot.c').read_text(),'it_802A4454').encode()).hexdigest(),
                'pull_reference_retail_dol_sha1':'08e0bf20134dfcb260699671004527b2d6bb1a45',
                'pull_reference_retail_body_sha256':'3c69097146cc00f8822e5caf96ca6b645acf25cf11d3759614f4ccd2b8b3d3e4',
                'sweep_body_sha256':hashlib.sha256(body((snapshot/'src/melee/it/kinds/itlinkhookshot.c').read_text(),'it_802A3D90').encode()).hexdigest(),
                'symbol_substitutions':{'it_802A3D90':'fixtureGrappleSweep','mpColl_800471F8':'fixtureSweepStage','it_802A4454':'fixtureMoveGrappleLine','mpCheckAllRemap':'fixtureSegmentBlocked','mpGetSpeed':'fixtureLineSpeed','mpLib_80054ED8':'fixtureLineEnabled','mpLib_8004D164':'fixtureGetMap','mpGetGroundCollLine':'fixtureGetLines','mpGetGroundCollVtx':'fixtureGetVertices'}}
    (out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({'status': status, 'results': results, 'receipt': str(out / 'receipt.json')}))
    print((result.stdout + result.stderr)[-6000:])
    raise SystemExit(result.returncode or (not unchanged) or (not base and not live_match))


if __name__ == '__main__':
    main()
