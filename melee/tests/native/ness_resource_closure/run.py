"""Console-only original-C comparison; never launches Melee or builds the tree."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess

HERE = Path(__file__).resolve().parent
ORIGINALS = {
    'lifecycle': {
        'src/melee/ft/kinds/ftNess/ftness.c': ['ftNs_Init_OnDeath', 'ftNs_Init_OnDamage'],
        'src/melee/ft/kinds/ftNess/ftnessattackhi4.c': [
            'ftNs_AttackHi4_YoyoItemSetUnk2', 'ftNs_AttackHi4_YoyoItemSetFlag',
            'ftNs_AttackHi4_YoyoItemDespawn', 'ftNs_AttackHi4_YoyoItemSetUnk',
            'ftNs_AttackHi4_YoyoCreateItem'],
        'src/melee/ft/kinds/ftNess/ftnessspecialn.c': ['ftNs_SpecialN_ItemPKFlushSetNULL'],
        'src/melee/ft/kinds/ftNess/ftnessspecialhi.c': ['ftNs_SpecialHi_TakeDamage'],
        'src/melee/ft/kinds/ftNess/ftnessattacks4.c': [
            'ftNs_AttackS4_ItemNessBatRemove', 'ftNs_AttackS4_Enter'],
        'src/melee/it/kinds/itnessyoyo.c': [
            'it_802BE958_inline', 'it_802BE958', 'it_802BE598', 'it_802BE5B8'],
        'src/melee/it/kinds/itnessbat.c': ['it_802AD6B8'],
        'src/melee/it/kinds/itnesspkflash.c': ['it_802AAA50'],
        'src/melee/it/kinds/itnesspkthunderball.c': ['it_802AB9C0'],
        'src/melee/it/kinds/itnesspkthundertrail.c': ['it_802AC5D8'],
        'src/melee/it/it_26B1.c': ['it_8026B724', 'it_8026B73C'],
    },
    'bat': {
        # The actual inline initializer must precede its calling constructor.
        'src/melee/it/kinds/inlines.h': ['Item_InitSpawnOnPlaneNoInitialCollision'],
        'src/melee/it/kinds/itnessbat.c': [
            'it_802AD478', 'it_802AD590', 'it_2725_Logic58_PickedUp'],
    },
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')


def function(text, name):
    match = re.search(r'(?m)^[^\n;{}]*\b' + re.escape(name) +
                      r'\([^;]*?\)\s*(?://[^\n]*\n\s*)?\{', text)
    if not match:
        raise RuntimeError(f'Original function not found: {name}')
    begin = text.index('{', match.start())
    end, depth = begin + 1, 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


def helper(text, name):
    match = re.search(r'(?m)^(?:export )?func ' + re.escape(name) + r'\([^\n]*\):\n', text)
    if not match:
        raise RuntimeError(f'Authored helper not found: {name}')
    end = re.search(r'(?m)^\S', text[match.end():])
    return text[match.start():match.end() + end.start()] if end else text[match.start():]


def lifecycle_source(candidate):
    life = (HERE / 'lifecycle.fms').read_text()
    prefix = ('state int flash_handle\nstate int thunder_ball_handle\n'
              'state bool thunder_effects_active\n' +
              life[life.index('state int bat_handle'):life.index('func resource_initialize')])
    for name in ['flash_clear', 'flash_detach', 'thunder_stop_effects', 'thunder_damage']:
        prefix += helper(candidate, name)
    for name in ['resource_initialize', 'bat_forget', 'bat_remove', 'yoyo_pause',
                 'yoyo_resume', 'yoyo_forget', 'yoyo_remove', 'resource_damage',
                 'bat_finish_construction', 'yoyo_finish_construction', 'bat_create', 'yoyo_create']:
        prefix += helper(candidate, name)
    prefix += life[life.index('action ResourceLifetime:'):]
    return prefix + (HERE / 'lifecycle_articles.fms').read_text()


def prepare(kind, directory, snapshot, manifest, candidate):
    directory.mkdir(parents=True, exist_ok=True)
    if kind == 'lifecycle':
        source = lifecycle_source(candidate)
        life = (HERE / 'lifecycle.fms').read_text()
        parameters = ('int thunder_ground_start = 358\nint thunder_air_rebound = 366\n'
                      'int thunder_trail_count = 6\n' +
                      ''.join(line[10:] + '\n' for line in life.splitlines()
                              if line.startswith('parameter ')))
    else:
        source = ('state int bat_handle\nfunc bat_forget():\n    state.bat_handle = -1\n'
                  'action Probe:\n    on load:\n        pass\n' + (HERE / 'bat.fms').read_text())
        parameters = ('int resource_attachment_joint = 42\nfloat bat_lifetime = 1200\n'
                      'int bat_smash_motion = 341\n')
    (directory / 'behavior.fms').write_text(source, encoding='utf-8')
    (directory / 'fighter_parameters.fmp').write_text(parameters, encoding='utf-8')
    shutil.copyfile(HERE / f'{kind}_compare.c', directory / 'compare.c')
    expected = json.loads((HERE / f'{kind}_original_inputs.json').read_text())
    originals, inputs = '', {}
    for path, names in ORIGINALS[kind].items():
        file = snapshot / 'sdk' / path
        actual = sha(file)
        if actual != expected[path]:
            raise RuntimeError(f'Original source differs from accepted30: {path}')
        verify_snapshot_file(file, snapshot, manifest)
        inputs[path] = actual
        text = file.read_text()
        for name in names:
            originals += function(text, name) + '\n'
    (directory / 'original.h').write_text(originals, encoding='utf-8')
    inputs['candidate.fms'] = sha(HERE / 'candidate.fms')
    write_json(directory / 'original_inputs.json', inputs)


def verify_snapshot_file(path, snapshot, manifest):
    key = path.relative_to(snapshot).as_posix()
    expected = manifest.get(key)
    if expected is None or sha(path) != expected:
        raise RuntimeError(f'Immutable snapshot hash mismatch/missing entry: {key}')


def run(command, log, cwd=None):
    result = subprocess.run(command, cwd=cwd, capture_output=True, text=True)
    log.write_text(result.stdout + result.stderr, encoding='utf-8')
    if result.returncode:
        raise RuntimeError(f'Command failed ({result.returncode}); see {log}')
    return result.stdout


def vcvars():
    installer = Path(os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)'))
    vswhere = installer / 'Microsoft Visual Studio/Installer/vswhere.exe'
    if not vswhere.is_file():
        raise RuntimeError('Visual Studio vswhere not found.')
    result = subprocess.run([str(vswhere), '-latest', '-products', '*', '-requires',
                             'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
                             '-property', 'installationPath'], capture_output=True, text=True, check=True)
    path = Path(result.stdout.strip()) / 'VC/Auxiliary/Build/vcvars64.bat'
    if not path.is_file():
        raise RuntimeError('Visual Studio vcvars64.bat not found.')
    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot', required=True, type=Path,
                        help='Immutable directory containing freeze.json, sdk, bin and compiler')
    parser.add_argument('--output', required=True, type=Path,
                        help='Scratch output directory outside the fixture/snapshot')
    args = parser.parse_args()
    snapshot, output = args.snapshot.resolve(), args.output.resolve()
    for protected in (HERE, snapshot):
        if output == protected or protected in output.parents:
            raise RuntimeError('Output must be outside tracked fixture and immutable snapshot.')
    manifest_file = snapshot / 'freeze.json'
    manifest = {key.replace('\\', '/'): value
                for key, value in json.loads(manifest_file.read_text()).items()}
    tool = snapshot / 'bin/native_content_tool.exe'
    clang = snapshot / 'compiler/clang.exe'
    # Verify all SDK sources/headers, not only originals; generated callbacks
    # must never silently compile through a modified or live include tree.
    for key in manifest:
        if key.startswith('sdk/') or key in ('bin/native_content_tool.exe', 'compiler/clang.exe'):
            verify_snapshot_file(snapshot / key, snapshot, manifest)
    verify_snapshot_file(tool, snapshot, manifest)
    verify_snapshot_file(clang, snapshot, manifest)
    output.mkdir(parents=True, exist_ok=True)
    candidate = (HERE / 'candidate.fms').read_text()
    expected_candidate = json.loads((HERE / 'lifecycle_original_inputs.json').read_text())['candidate.fms']
    if sha(HERE / 'candidate.fms') != expected_candidate:
        raise RuntimeError('Candidate differs from preserved30 receipt.')
    setup = vcvars()
    results = {}
    for kind, expected_checks in [('lifecycle', 67270), ('bat', 122)]:
        directory = output / kind
        prepare(kind, directory, snapshot, manifest, candidate)
        run([str(tool), 'script-c', str(directory/'behavior.fms'), f'ness_resource_{kind}',
             str(directory/'behavior.generated.h')], directory/'generate.log')
        command = [str(clang), '-std=gnu11', '-O1', '-ffp-contract=off', '-DMELEE_NATIVE=1',
                   '-D_CRT_SECURE_NO_WARNINGS', '-D_USE_MATH_DEFINES', '-D_DLL', '-D_MT',
                   '-Xclang', '--dependent-lib=msvcrt', '-I'+str(snapshot/'sdk/src'),
                   '-I'+str(snapshot/'sdk/extern/dolphin/include'), str(directory/'compare.c'),
                   '-fuse-ld=lld', '-o', str(directory/'compare.exe')]
        compile_text = ('@echo off\ncall "'+str(setup)+'" -vcvars_ver=14.44 >nul\n'
                        'if errorlevel 1 exit /b 1\n'+subprocess.list2cmdline(command)+'\n')
        (directory/'compile.cmd').write_text(compile_text, encoding='utf-8')
        run(['cmd.exe', '/d', '/c', str(directory/'compile.cmd')], directory/'compile.log')
        text = run([str(directory/'compare.exe')], directory/'result.log')
        match = re.search(r'(\d+) checks, (\d+) failures', text)
        if not match or tuple(map(int, match.groups())) != (expected_checks, 0):
            raise RuntimeError(f'Unexpected comparison scope: {text}')
        results[kind] = {'checks': expected_checks, 'failures': 0,
                         'original_inputs_sha256': sha(directory/'original_inputs.json'),
                         'generated_sha256': sha(directory/'behavior.generated.h'),
                         'result_sha256': sha(directory/'result.log')}
        print(text.strip())
    write_json(output/'receipt.json', {
        'status': 'source-only; unregistered; YoYo initial-position seed pending',
        'snapshot': str(snapshot), 'snapshot_receipt_sha256': sha(manifest_file),
        'tool_sha256': sha(tool), 'clang_sha256': sha(clang),
        'fixture_sources': {path.name: sha(path) for path in sorted(HERE.iterdir()) if path.is_file()},
        'results': results,
    })
    print(f'Receipt: {output / "receipt.json"}')


if __name__ == '__main__':
    main()
