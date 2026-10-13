"""Compare original Captain/Ganon policies with matching generated modules.

Only the standalone fixture is compiled. No game or global build is launched.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
ORIGINAL = '4c76e0752'
ATTACK = 'src/melee/ft/ftcpuattack.c'
SWING = 'src/melee/ft/ftswing.c'
TAUNT = 'src/melee/ft/kinds/ftCommon/ftCo_AppealS.c'
CAPTAIN = 'src/melee/ft/kinds/ftCaptain/ftcaptain.c'
COMMON = 'src/melee/ft/kinds/ftCommon/ftCo_0A01.c'


def function(text, name):
    pattern = r'^(?:static inline |static )?(?:void|bool|int) ' + name + r'\([^;]*?\)\s*\{'
    match = re.search(pattern, text, re.MULTILINE)
    if not match:
        raise AssertionError('Function missing: ' + name)
    start = match.end() - 1
    depth, end = 1, start + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


def excerpts(batch, sdk):
    originals = {path: subprocess.check_output(['git', 'show', ORIGINAL + ':' + path],
                    cwd=ROOT, text=True) for path in (ATTACK, SWING, TAUNT, CAPTAIN, COMMON)}
    text = '// Original source ' + ORIGINAL + '; function names isolated only.\n'
    for name in ('ftCo_CpuClearTargetAndFinish', 'ftCo_CpuSetNeutralStick',
                 'ftCo_CpuFlickLstickX', 'ftCo_CpuPressAWithLstickX'):
        text += function(originals[ATTACK], name)
    text += function(originals[ATTACK], 'ftCo_800B683C').replace('ftCo_800B683C', 'original_capture')
    text += function((sdk / ATTACK).read_text(), 'ftCo_800B683C').replace('ftCo_800B683C', 'production_capture')
    begin = originals[ATTACK].index('    case Ft_Kind_Captain:', originals[ATTACK].index('void ftCo_800B77E8'))
    end = originals[ATTACK].index('    case Ft_Kind_Pikachu:', begin)
    text += 'static void original_ranged(Fighter* fp) {\nFighter* target; Fighter** target_pp; bool can_attack; switch(fp->kind) {\n'
    text += originals[ATTACK][begin:end] + 'default: break; } }\n'
    text += function(originals[TAUNT], 'ftCo_800DEBD0')
    text += function(originals[TAUNT], 'ftCo_800DEA28').replace('ftCo_800DEA28', 'original_taunt')
    table = re.search(r'int lbl_803C6D70\[6\]\[5\] = \{.*?\n\};', originals[SWING], re.S)
    assert table, 'Common swing matrix missing'
    text += table[0] + '\n'
    text += function(originals[SWING], 'get_anim_id').replace('get_anim_id', 'original_get_anim_id')
    text += function((sdk / SWING).read_text(), 'get_anim_id').replace('get_anim_id', 'production_get_anim_id')
    text += function(originals[COMMON], 'ftCo_800A0CB0').replace('ftCo_800A0CB0', 'original_spacing')
    text += function((sdk / COMMON).read_text(), 'ftCo_800A0CB0').replace('ftCo_800A0CB0', 'production_spacing')
    motion_table = originals[CAPTAIN].split('MotionState ftCa_Init_MotionStateTable', 1)[1].split('\n};', 1)[0]
    phase_rows = re.findall(r'\{\s*//[^\n]*\n(.*?)\n    \}', motion_table, re.S)
    masks = []
    for row in phase_rows:
        fields = [field.strip() for field in row.split(',') if field.strip()]
        assert len(fields) == 8, fields
        masks.append(sum(1 << phase for phase, field in enumerate(fields[3:7]) if field != 'NULL'))
    assert len(masks) == 23, masks
    text += 'static const unsigned original_phase_masks[23] = {' + ','.join(map(str, masks)) + '};\n'
    (batch / 'captain_ganon_original.inc').write_text(text)
    return originals


def unused_host(batch, sdk, projects):
    """ABI-exact traps for full-module callbacks this fixture never executes."""
    required = set()
    for project in projects:
        required.update(re.findall(r'host->(\w+) == 0', (project / 'behavior.generated.c').read_text()))
    provided = {'cpu_ranged_unsafe', 'cpu_add_candidate', 'cpu_run_script', 'random_float',
        'callback_source_alive', 'common_motion_states', 'resolve_bone', 'joint_position',
        'spawn_wind', 'enter_taunt', 'change_motion_state', 'start_animation'}
    header = (sdk / 'src/melee/ft/native_fighter_module.h').read_text()
    header = header.split('typedef struct NativeFighterHost {', 1)[1].split('} NativeFighterHost;', 1)[0]
    prototypes = {match[2]: (match[1].strip(), match[3].strip()) for match in
        re.finditer(r'^\s*([^;\n]+?)\(\*(\w+)\)\((.*?)\);', header, re.M | re.S)}
    for name in re.findall(r'^\s*HSD_GObjEvent (\w+);', header, re.M):
        prototypes[name] = ('void', 'HSD_GObj*')
    output, assignments = [], []
    for name in sorted(required - provided):
        assert name in prototypes, 'Missing actual Host prototype: ' + name
        result, arguments = prototypes[name]
        parts, depth, begin = [], 0, 0
        for offset, char in enumerate(arguments):
            depth += (char == '(') - (char == ')')
            if char == ',' and depth == 0:
                parts.append(arguments[begin:offset].strip()); begin = offset + 1
        parts.append(arguments[begin:].strip())
        named = []
        for index, argument in enumerate(parts):
            if argument in ('void', '...'):
                named.append(argument)
            elif '(*)' in argument:
                named.append(argument.replace('(*)', '(*unused_' + str(index) + ')'))
            else:
                named.append(argument + ' unused_' + str(index))
        output.append('static ' + result + ' trap_' + name + '(' + ','.join(named) + ')\n'
                      '{ require(false, "Unexpected Host call: ' + name + '"); abort(); }\n')
        assignments.append('    host->' + name + ' = trap_' + name + ';\n')
    output.append('static void supply_unused_host(NativeFighterHost* host)\n{\n' + ''.join(assignments) + '}\n')
    (batch / 'captain_ganon_unused_host.inc').write_text(''.join(output))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--batch', required=True, type=Path)
    parser.add_argument('--snapshot', required=True, type=Path)
    parser.add_argument('--captain-project', required=True, type=Path)
    parser.add_argument('--ganon-project', required=True, type=Path)
    args = parser.parse_args()
    batch, snapshot = args.batch.resolve(), args.snapshot.resolve()
    projects = [args.captain_project.resolve(), args.ganon_project.resolve()]
    batch.mkdir(parents=True, exist_ok=True)
    sdk = snapshot / 'sdk'
    originals = excerpts(batch, sdk)
    unused_host(batch, sdk, projects)
    executable = batch / 'captain_ganon_policy_stock_test.exe'
    command = [str(snapshot / 'compiler/clang.exe'), '-fms-runtime-lib=dll', '-std=gnu11', '-O2',
        '-DMELEE_NATIVE=1', '-D_USE_MATH_DEFINES', '-D_CRT_SECURE_NO_WARNINGS', '-D_DLL', '-D_MT',
        '-I' + str(sdk / 'src'), '-I' + str(sdk / 'extern/dolphin/include'), '-I' + str(batch),
        str(ROOT / 'tests/native/captain_ganon_policy_stock_test.c'),
        str(sdk / 'src/melee/ft/native_fighter_parameters.c'), '-o', str(executable)]
    setup = Path('C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat')
    script = batch / 'compile-policy-fixture.cmd'
    script.write_text('@echo off\ncall ' + subprocess.list2cmdline([str(setup)]) + ' >nul\n' +
                      subprocess.list2cmdline(command) + '\n')
    with (batch / 'policy-fixture-build.log').open('wb') as log:
        result = subprocess.run(['cmd', '/c', str(script)], cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
    if result.returncode:
        print((batch / 'policy-fixture-build.log').read_text())
        return result.returncode
    run = [str(executable)]
    for project in projects:
        run += [str(project / 'module.dll'), str(project / 'fighter_parameters.fmp')]
    result = subprocess.run(run, cwd=ROOT, text=True, capture_output=True)
    (batch / 'policy-fixture-results.log').write_text(result.stdout + result.stderr)
    inputs = [ROOT / 'tests/native/captain_ganon_policy_stock_test.c',
              batch / 'captain_ganon_original.inc']
    for project in projects:
        inputs += [project / 'module.dll', project / 'fighter_parameters.fmp', project / 'behavior.fms']
    inputs += [sdk / path for path in (ATTACK, SWING, TAUNT, COMMON)]
    evidence = {'original_commit': ORIGINAL, 'snapshot': str(snapshot),
        'exit_code': result.returncode, 'output': result.stdout + result.stderr,
        'original_sha256': {path: hashlib.sha256(text.encode()).hexdigest() for path, text in originals.items()},
        'sha256': {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in inputs}}
    (batch / 'policy-fixture-result.json').write_text(json.dumps(evidence, indent=2) + '\n')
    print(result.stdout + result.stderr, end='')
    return result.returncode


if __name__ == '__main__':
    raise SystemExit(main())
