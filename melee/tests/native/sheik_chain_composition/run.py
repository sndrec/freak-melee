"""Compare complete unregistered Chain policy with pinned original C bodies."""
import argparse
import hashlib
import importlib.util
import json
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('root', 'snapshot', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--iso', type=Path)
    parser.add_argument('--corrected-chain-source', type=Path,
                        default=HERE / 'corrected_control_actual.h')
    args = parser.parse_args()
    root, snapshot, output = args.root.resolve(), args.snapshot.resolve(), args.output.resolve()
    sdk = snapshot / 'sdk'
    tool, clang = snapshot / 'bin/native_content_tool.exe', snapshot / 'compiler/clang.exe'
    iso = args.iso or root.parent / 'Super Smash Bros. Melee (USA) (v1.02).iso'
    commands = []

    def run(command, log):
        command = list(map(str, command))
        commands.append(command)
        process = subprocess.run(command, cwd=root, capture_output=True, text=True)
        (output / log).write_text(process.stdout + process.stderr, encoding='utf-8')
        if process.returncode:
            raise RuntimeError(f'{log}: exit {process.returncode}\n' + (process.stdout + process.stderr)[-9000:])
        return process.stdout

    preparation_command = list(map(str, [sys.executable, HERE / 'prepare.py', '--root', root,
        '--snapshot', snapshot, '--iso', iso, '--output', output]))
    commands.append(preparation_command)
    prepared = subprocess.run(preparation_command, cwd=root, capture_output=True, text=True)
    if not output.exists():
        raise RuntimeError(prepared.stdout + prepared.stderr)
    (output / 'prepare.log').write_text(prepared.stdout + prepared.stderr, encoding='utf-8')
    if prepared.returncode:
        raise RuntimeError(prepared.stdout + prepared.stderr)
    run([tool, 'script-c', output / 'owned.fms', 'sheik_chain_source', output / 'generated.inc'], 'generate.log')
    preparation = json.loads((output / 'preparation.json').read_text())
    parameters = preparation['parameters']
    generated = (output / 'generated.inc').read_text()
    schema = re.search(r'count != (\d+) \|\| schema != (0x[0-9a-fA-F]+)ULL', generated)
    assert schema and int(schema[1]) == len(parameters)
    (output / 'parameters.inc').write_text(
        'static const uint32_t parameter_bits[]={' + ','.join(f"UINT32_C({p['bits']})" for p in parameters) + '};\n'
        + f'static const uint64_t parameter_schema={schema[2]}ULL;\n'
        + 'static const char*parameter_names[]={' + ','.join(json.dumps(p['name']) for p in parameters) + '};\n',
        encoding='utf-8')
    policy_attributes = []
    article_attributes = []
    for parameter in parameters:
        offset, slot = parameter['source_offset'], parameter['article_slot']
        if offset is None:
            continue
        statement = f"{{ uint32_t bits=UINT32_C({parameter['bits']}); memcpy((uint8_t*)&attributes+{offset},&bits,4); }}"
        if slot == 3:
            article_attributes.append(statement)
            policy_attributes.append(statement.replace('&attributes', '&chain_storage'))
        elif slot is None:
            policy_attributes.append(statement)
    (output / 'policy_attributes.inc').write_text('\n'.join(policy_attributes) + '\n', encoding='utf-8')
    (output / 'article_attributes.inc').write_text('\n'.join(article_attributes) + '\n', encoding='utf-8')

    spec = importlib.util.spec_from_file_location('needle_extraction', HERE.parent / 'sheik_script_composition/needle_fixture.py')
    extraction = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(extraction)
    body = extraction.body
    fighter = (sdk / 'src/melee/ft/kinds/ftSeak/ftseakspecials.c').read_text()
    original_helpers = ['ftSk_SpecialS_80110490', 'ftSk_SpecialS_80110610', 'ftSk_SpecialS_80110788',
        'ftSk_SpecialS_UpdateHitboxes', 'ftSk_SpecialS_ZeroHitboxPositions', 'ftSk_SpecialS_80110AEC',
        'ftSeakSpecialS_LoopChainHitCollisions', 'ftSeakSpecialS_LoopChainHitActivate', 'sumOfSquares', 'ftSk_SpecialS_80110BCC']
    (output / 'fighter_policy_actual.inc').write_text('#define COMMON_DATA_F32 ((float*)p_ftCommonData)\n'
        + '\n\n'.join(body(fighter, name) for name in original_helpers) + '\n', encoding='utf-8')
    phases = fighter[fighter.index('void ftSk_SpecialS_80110F70('):fighter.index('bool ftSk_SpecialS_80111F70(')]
    phase_helpers = ['ftSk_SpecialS_8011097C', 'ftSk_SpecialS_80110E4C', 'ftSk_SpecialS_CheckAndDestroyChain',
                     'ftSk_SpecialS_80110EE8', 'ftSk_SpecialS_ChainSomething']
    (output / 'fighter_phase_actual.inc').write_text(phases + '\n\n'.join(body(fighter, name) for name in phase_helpers) + '\n', encoding='utf-8')
    needle = (sdk / 'src/melee/ft/kinds/ftSeak/ftseakspecialn.c').read_text()
    init = (sdk / 'src/melee/ft/kinds/ftSeak/ftseak.c').read_text()
    (output / 'fighter_cleanup_actual.inc').write_text('\n\n'.join([
        body(needle, 'ftSk_SpecialN_80111FBC'), body(needle, 'ftSk_SpecialN_801120D4'),
        body(init, 'ftSk_Init_80110198')]) + '\n', encoding='utf-8')

    article = (sdk / 'src/melee/it/kinds/itseakchain.c').read_text()
    hookshot = (sdk / 'src/melee/it/kinds/itlinkhookshot.c').read_text()
    inlines = (sdk / 'src/melee/it/kinds/inlines.h').read_text()
    old_control = body(article, 'it_802BC080')
    corrected_path = args.corrected_chain_source.resolve()
    corrected_control = body(corrected_path.read_text(encoding='utf-8'), 'it_802BC080')
    old_read, new_read = '*(s32*) &fp->mv.co.common.x1C', 'fp->mv.sk.specials.x1C'
    tokens = lambda text: re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S).split()
    assert old_control.count(old_read) == 1
    assert tokens(old_control.replace(old_read, new_read)) == tokens(corrected_control), 'Correction changes more than typed timer read'
    corrected_inputs = output / 'corrected_original_inputs'
    corrected_inputs.mkdir()
    (corrected_inputs / 'itseakchain.c').write_bytes(corrected_path.read_bytes())
    (output / 'article_solver_native64_control.inc').write_text(old_control.replace('it_802BC080(', 'it_802BC080_native64(', 1) + '\n', encoding='utf-8')
    article_names = ['it_802BB938', 'it_802BBAEC', 'it_802BBB0C', 'it_802BBC38', 'it_802BBD64',
        'it_802BBED0', 'itSeakChain_clamp_x10', 'itSeakChain_clamp_x14', 'it_802BC080', 'it_802BC94C']
    solver_bodies = [body(hookshot, name) for name in ['it_802A3C98', 'it_802A43B8', 'it_802A43EC', 'it_802A4420']]
    solver_bodies += [body(inlines, 'Item_RetractChain'), body(fighter, 'ftSk_SpecialS_80110F58'), body(fighter, 'ftSk_SpecialS_80110F64')]
    solver_bodies += [corrected_control if name == 'it_802BC080' else body(article, name) for name in article_names]
    (output / 'article_solver_actual.inc').write_text('\n\n'.join(solver_bodies) + '\n', encoding='utf-8')
    runtime_names = ['inlineA0', 'it_802BB20C', 'fn_802BB428', 'fn_802BB44C', 'fn_802BB574', 'fn_802BB694',
        'fn_802BB784', 'notInSpecialS', 'itSeakchain_UnkMotion4_Anim', 'it_802BCA30', 'it_802BCB88_prev',
        'it_802BCB88', 'it_2725_Logic54_PickedUp', 'it_802BCED4', 'it_802BCF2C', 'it_802BCF84',
        'it_802BCFC4', 'itSeakChain_Logic54_EvtUnk']
    runtime_bodies = [body((sdk / 'src/melee/lb/lbvector.c').read_text(), 'lbVector_Normalize'), body(hookshot, 'it_802A6DC8')]
    runtime_bodies += [body(fighter, name) for name in ['ftSk_SpecialS_UpdateHitboxes', 'ftSk_SpecialS_80110E4C', 'ftSk_SpecialS_ChainSomething']]
    runtime_bodies += [body(article, name) for name in runtime_names]
    (output / 'article_runtime_actual.inc').write_text('\n\n'.join(runtime_bodies) + '\n', encoding='utf-8')
    constructor_names = ['load_chain_link_model', 'load_chain_tip_model', 'free_chain_links', 'create_chain_links', 'itSeakChain_Spawn']
    constructor_bodies = [body(inlines, 'Item_InitSpawn'), body((sdk / 'src/melee/it/it_2725.c').read_text(), 'it_80275328'), body(hookshot, 'it_802A24D0')]
    constructor_bodies += [body(article, name) for name in constructor_names]
    (output / 'article_constructor_actual.inc').write_text('\n\n'.join(constructor_bodies) + '\n', encoding='utf-8')
    helper_names = re.findall(r'^    func (\w+)\(', (HERE / 'article.fms').read_text(), re.M)
    aliases = []
    for name in ('Extend', 'Control', 'Retract', 'Render', 'TipAtHand', 'Cleanup'):
        index = helper_names.index(name)
        emitted = re.search(r'^static [^\n;]+ (scripted_article_2_(?:mutating_(?:value_)?)?helper_' + str(index) + r')\([^;\n]+\) \{', generated, re.M)
        assert emitted, name
        aliases.append(f'#define chain_{name} {emitted[1]}')
    (output / 'article_names.inc').write_text('\n'.join(aliases) + '\n', encoding='utf-8')

    flags = [clang, '-std=gnu11', '-O2', '-ffunction-sections', '-fdata-sections', '-fms-runtime-lib=dll',
        '-DMELEE_NATIVE=1', '-D_DLL', '-D_MT', '-D_USE_MATH_DEFINES', '-D_CRT_SECURE_NO_WARNINGS',
        '-I', sdk / 'src', '-I', sdk / 'extern/dolphin/include', '-I', output]

    def compile_source(command, name):
        batch = output / (name + '.cmd')
        batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" -vcvars_ver=14.44 >nul\n'
                         + subprocess.list2cmdline(list(map(str, command))) + '\n', encoding='utf-8')
        run(['cmd', '/c', batch], name + '.log')

    compile_source(flags + ['-shared', '-x', 'c', output / 'generated.inc', '-o', output / 'module.dll'], 'module')
    results = []
    for name, pattern in [('fighter_policy', 'policy'), ('article_solver', 'article solver'), ('article_runtime', 'article runtime')]:
        executable = output / (name + '.exe')
        compile_source(flags + [HERE / (name + '_test.c'), sdk / 'src/melee/ft/native_callback_lifetime.c',
                       '-o', executable, '-Wl,/OPT:REF'], name + '_compile')
        result = run([executable], name + '.log')
        match = re.search(r'PASS (\d+) ' + pattern + r' pairs, (\d+) checks, maximum_float_difference (\S+)', result)
        assert match, result
        results.append((int(match[1]), int(match[2]), float(match[3])))
        print(result.strip())
    policy, solver, runtime = results
    receipt = {
        'status': 'PASS complete Chain policy source gate; retail assets, registration and gameplay unaccepted',
        'snapshot': str(snapshot), 'freeze_sha256': sha(snapshot / 'freeze.json'),
        'policy_pairs': policy[0], 'checks': policy[1], 'maximum_float_difference': policy[2],
        'article_solver_pairs': solver[0], 'article_solver_checks': solver[1], 'article_maximum_float_difference': solver[2],
        'article_runtime_pairs': runtime[0], 'article_runtime_checks': runtime[1], 'article_runtime_maximum_float_difference': runtime[2],
        'total_pairs': sum(result[0] for result in results), 'total_checks': sum(result[1] for result in results),
        'corrected_reference': {'source': str(corrected_path), 'sha256': sha(corrected_path),
            'body_sha256': hashlib.sha256(corrected_control.encode('utf-8')).hexdigest(), 'function': 'it_802BC080',
            'native64_failure_archive': 'A:/FreakMelee-test-archives/sheik-chain-solvers64-r1',
            'offsets_from_mv': {'co.common.x1C': 32, 'sk.specials.x1C': 28, 'sk.specials.x20': 32},
            'change': 'Only typed Sheik rehit read changes; remaining original body tokens identical.'},
        'original_helpers': original_helpers, 'preparation': preparation, 'commands': commands,
        'inputs': {str(path.relative_to(root)): sha(path) for path in sorted(HERE.iterdir()) if path.is_file()},
        'generated_inputs': {path.name: sha(path) for path in sorted(output.iterdir()) if path.suffix in ('.inc', '.fms', '.fmp', '.dll', '.exe')},
        'frozen_tools': {'native_content_tool': sha(tool), 'clang': sha(clang)},
        'limits': ['Unregistered source candidate, not a whole independent Sheik.',
            'Retail costume/pose graph/animation bank/model import and rendered gameplay are not accepted.',
            'Shared pose, stage contacts, constructor allocation/model activation, notification and joint/resource boundaries are controlled.',
            'Source uninitialized previous rehit and empty-propagation env_flags have documented deterministic initialization; undefined missing-Chain paths excluded.',
            'Malformed counts and model activation failures are separately defined safety controls, not original defined-behavior pairs.']}
    (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    print(output / 'receipt.json')


if __name__ == '__main__':
    main()
