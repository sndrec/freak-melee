"""Fresh-import and directly compile the unregistered Ness smash source comparison.

Console only. No game, global build, or frozen-tool script-build invocation.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import subprocess

HERE = Path(__file__).resolve().parent
STOCK = [
    'src/melee/ft/kinds/ftNess/ftnessattacks4.c',
    'src/melee/ft/kinds/ftNess/ftnessattackhi4.c',
    'src/melee/ft/kinds/ftNess/ftnessattacklw4.c',
    'src/melee/ft/ft_0DF0.c',
]
HELPERS = {
    'src/melee/lb/lbcollision.c': ['lbColl_80008428', 'lbColl_80008434', 'lbColl_80008440'],
    'src/melee/ft/ftcoll.c': ['ftColl_8007B8A8', 'ftColl_8007AFF8', 'ftColl_8007B064'],
    'src/melee/lb/lbvector.c': ['lbVector_Add', 'lbVector_Sub', 'lbvector_sin',
                             'lbvector_cos', 'lbVector_Rotate'],
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def dump(path, value):
    path.write_text(json.dumps(value, indent=2)+'\n', encoding='utf-8')


def extract(text, name):
    match = re.search(r'(?m)^[A-Za-z_][^\n;{}]*\b'+re.escape(name)+
                      r'\([^;]*?\)\s*(?://[^\n]*\n\s*)?\{', text)
    if not match:
        raise RuntimeError('Original function not found: '+name)
    opening = text.index('{', match.start())
    end, depth = opening+1, 1
    while depth:
        depth += (text[end] == '{')-(text[end] == '}')
        end += 1
    return text[match.start():end]+'\n\n'


def run(command, log):
    result = subprocess.run(command, capture_output=True, text=True)
    log.write_text(result.stdout+result.stderr, encoding='utf-8')
    if result.returncode:
        raise RuntimeError(f'Failed({result.returncode}): {log}\n'+result.stdout+result.stderr)
    return result.stdout


def cleanup_fragment(text):
    start = text.index('    if ((flags & Ft_MF_SkipHit) == 0) {',
                       text.index('void Fighter_ChangeMotionState('))
    opening = text.index('{', start)
    end, depth = opening+1, 1
    while depth:
        depth += (text[end] == '{')-(text[end] == '}')
        end += 1
    return ('static void transition_hitbox_cleanup(Fighter_GObj* gobj, MotionFlags flags)\n'
            '{\n    Fighter* fp = GET_FIGHTER(gobj);\n'+text[start:end]+'\n}\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--iso', required=True, type=Path)
    args = parser.parse_args()
    snapshot, output, iso = args.snapshot.resolve(), args.output.resolve(), args.iso.resolve()
    if any(output == p or p in output.parents for p in [snapshot, HERE]):
        raise RuntimeError('Output cannot modify snapshot or tracked source.')
    frozen = {k.replace('\\', '/'): v for k, v in
              json.loads((snapshot/'freeze.json').read_text()).items()}
    for key, value in frozen.items():
        if key.startswith(('sdk/', 'compiler/')) or key == 'bin/native_content_tool.exe':
            if sha(snapshot/key) != value:
                raise RuntimeError('Snapshot hash mismatch: '+key)
    expected = json.loads((HERE/'original_inputs.json').read_text())
    inputs = {}
    for path in expected:
        inputs[path] = sha(snapshot/'sdk'/path)
        if inputs[path] != expected[path]:
            raise RuntimeError('Original reference changed: '+path)
    components = json.loads((HERE/'component_inputs.json').read_text())
    for path, value in components.items():
        source = HERE.parent/path
        if sha(source) != value:
            raise RuntimeError('Accepted component changed: '+str(source))
    output.mkdir(parents=True, exist_ok=True)
    project = output/'project'
    if project.exists():
        raise RuntimeError('Use a fresh output: '+str(project))
    tool = snapshot/'bin/native_content_tool.exe'
    run([str(tool), 'clone-stock', str(iso), 'Ness', str(project),
         'Unregistered Ness smash comparison'], output/'clone.log')
    run([str(tool), 'import-parameters', str(project), str(HERE/'candidate.fms'),
         str(project/'behavior.fms')], output/'import.log')
    body = (project/'behavior.fms').read_text()
    if not re.search(r'^base ', body, re.M):
        body = 'base ftDataNess 341 35\n'+body
        (project/'behavior.fms').write_text(body, encoding='utf-8')
    run([str(tool), 'script-c', str(project/'behavior.fms'), 'ness_smash_composition',
         str(project/'behavior.generated.c')], output/'generate_full.log')
    # Keep the full imported FMP; its qualified article parameters are immutable
    # owned inputs to fighter decisions even when article callbacks run separately.
    fighter = output/'fighter'
    fighter.mkdir(exist_ok=True)
    (fighter/'fighter_parameters.fmp').write_bytes((project/'fighter_parameters.fmp').read_bytes())
    (fighter/'behavior.fms').write_text(body.split('\narticle ', 1)[0]+'\n', encoding='utf-8')
    run([str(tool), 'script-c', str(fighter/'behavior.fms'), 'ness_smash_composition',
         str(fighter/'behavior.generated.c')], output/'generate_fighter.log')
    generated = (fighter/'behavior.generated.c').read_text()
    required = sorted(set(re.findall(r'host->(\w+) == 0', generated)))
    (fighter/'required_host.h').write_text('\n'.join(
        'host.'+name+'=(__typeof__(host.'+name+'))unexpected;' for name in required)+'\n',
        encoding='utf-8')
    original = ''.join(extract((snapshot/'sdk'/path).read_text(), name)
                       for path, names in HELPERS.items() for name in names)
    (fighter/'original_helpers.h').write_text(original, encoding='utf-8')
    (fighter/'motion_cleanup.h').write_text(cleanup_fragment(
        (snapshot/'sdk/src/melee/ft/fighter.c').read_text()), encoding='utf-8')
    vswhere = Path(os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)'))/\
        'Microsoft Visual Studio/Installer/vswhere.exe'
    install = subprocess.run([str(vswhere), '-latest', '-products', '*', '-requires',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'],
        capture_output=True, text=True, check=True).stdout.strip()
    setup = Path(install)/'VC/Auxiliary/Build/vcvars64.bat'
    clang = snapshot/'compiler/clang.exe'
    flags = [str(clang), '-std=gnu11', '-O1', '-ffp-contract=off', '-ffunction-sections',
             '-fdata-sections', '-DMELEE_NATIVE=1', '-D_CRT_SECURE_NO_WARNINGS',
             '-D_USE_MATH_DEFINES', '-D_DLL', '-D_MT', '-Xclang', '--dependent-lib=msvcrt',
             '-I'+str(snapshot/'sdk/src'), '-I'+str(snapshot/'sdk/extern/dolphin/include')]
    commands = [flags+['-c', str(project/'behavior.generated.c'), '-o', str(output/'candidate.obj')],
                flags+['-I'+str(fighter), '-I'+str(HERE), str(HERE/'compare.c')]+
                [str(snapshot/'sdk'/path) for path in STOCK]+
                [str(snapshot/'sdk/src/melee/ft/native_fighter_parameters.c'),
                 '-fuse-ld=lld', '-Wl,/OPT:REF', '-o', str(fighter/'compare.exe')]]
    batch = output/'compile.cmd'
    batch.write_text('@echo off\ncall "'+str(setup)+'" -vcvars_ver=14.44 >nul\n'+
        'if errorlevel 1 exit /b 1\n'+''.join(subprocess.list2cmdline(command)+
        '\nif errorlevel 1 exit /b 1\n' for command in commands), encoding='utf-8')
    run(['cmd.exe', '/d', '/c', str(batch)], output/'compile.log')
    result = run([str(fighter/'compare.exe'), str(fighter/'fighter_parameters.fmp')], output/'result.log')
    match = re.search(r'(\d+) checks, (\d+) failures, (\d+) phase source pairs, (\d+) entry pairs, (\d+) helper pairs, (\d+) common-update pairs', result)
    if not match or tuple(map(int,match.groups()[1:])) != (0,3584,896,1792,256):
        raise RuntimeError('Unexpected comparison scope: '+result)
    safety = re.search(r'Authored-only lifetime: (\d+) removed/recycled source cases', result)
    if not safety or int(safety[1]) != 10:
        raise RuntimeError('Lifetime coverage gate failed: '+result)
    cleanup = re.search(r'Cleanup: (\d+) explicit latch deltas; (\d+) regression cases', result)
    if not cleanup or tuple(map(int,cleanup.groups())) != (180,30):
        raise RuntimeError('Cleanup regression gate failed: '+result)
    dump(output/'receipt.json', {
        'status': 'unregistered seven-state smash source composition only',
        'snapshot': str(snapshot), 'snapshot_sha256': sha(snapshot/'freeze.json'),
        'iso_sha256': sha(iso), 'originals': inputs, 'components': components,
        'fixture_sources': {p.name: sha(p) for p in HERE.iterdir() if p.is_file()},
        'imported_sha256': sha(project/'behavior.fms'),
        'parameters_sha256': sha(project/'fighter_parameters.fmp'),
        'full_generated_sha256': sha(project/'behavior.generated.c'),
        'fighter_generated_sha256': sha(fighter/'behavior.generated.c'),
        'original_helpers_sha256': sha(fighter/'original_helpers.h'),
        'motion_cleanup_fragment_sha256': sha(fighter/'motion_cleanup.h'),
        'results': {'checks': int(match[1]), 'failures': 0, 'phase_pairs': int(match[3]),
                    'entry_pairs': int(match[4]), 'helper_pairs': int(match[5]),
                    'common_update_pairs': int(match[6]), 'edited_tuning_sets': 1,
                    'authored_only_lifetime_cases': int(safety[1]),
                    'intentional_cleanup_latch_deltas': int(cleanup[1]),
                    'cleanup_regression_cases': int(cleanup[2])},
        'boundaries': ['controlled transitions and animation evaluator',
                       'ordered sound/color/rumble presentation',
                       'article constructor/signal decisions against accepted component fixtures',
                       'stage sweep descriptor and short-circuit behavior; no actual geometry solver'],
        'pending': ['whole Ness Flash/Thunder/YoYo/Bat lifecycle composition',
                    'independence/default activation', 'rendered gameplay'],
        'intentional_behavior_correction': 'generic hitbox enabling latches ordinary motion cleanup; scoped actual cleanup block regression, not whole Fighter_ChangeMotionState'})
    print(result.strip())
    print('Receipt:', output/'receipt.json')


if __name__ == '__main__':
    main()
