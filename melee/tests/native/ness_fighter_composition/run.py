"""Normal Ness asset import and source-only full composition generation.

No game or tree build. Frozen tool script-build is deliberately not used: it
embeds live SDK include paths. Generated C is compiled with the supplied SDK.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

from census import census
from compose import compose

HERE = Path(__file__).resolve().parent


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, log):
    result = subprocess.run(command, capture_output=True, text=True)
    log.write_text(result.stdout + result.stderr, encoding='utf-8')
    if result.returncode:
        raise RuntimeError(f'Failed ({result.returncode}): {log}\n' + result.stdout + result.stderr)
    return result.stdout


def extract(text, name):
    match = re.search(r'(?m)^[A-Za-z_][^\n;{}]*\b' + re.escape(name) +
                      r'\([^;]*?\)\s*(?://[^\n]*\n\s*)?\{', text)
    if not match:
        raise RuntimeError('Original function not found: ' + name)
    opening = text.index('{', match.start())
    end, depth = opening + 1, 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n\n'


def declaration(text, marker):
    start = text.index(marker)
    opening = text.index('{', start)
    end, depth = opening + 1, 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + ';\n'


def prepare_comparisons(snapshot, output, generated):
    originals = {
        'src/melee/ft/ftcmdscript.c': ['ftCo_800B463C', 'ftCo_800B46B8', 'ftCo_800B4778'],
        'src/melee/ft/ftcpuattack.c': ['ftCo_CpuClearTargetAndFinish', 'ftCo_800B920C',
                                     'ftCo_800B92D4'],
        'src/melee/ft/kinds/ftCommon/ftCo_0A01.c': ['ftCo_800A0C8C', 'angle_x_units',
            'angle_y_units', 'ftCo_CpuAimPkThunder', 'ftCo_CpuStartUpSpecial', 'ftCo_800A8EB0'],
    }
    # Define the common finish before the two original decisions which invoke it.
    bodies = extract((snapshot/'sdk/src/melee/ft/kinds/ftCommon/ftCo_0A01.c').read_text(),
                     'ftCo_800A0C8C')
    for path, names in originals.items():
        text = (snapshot/'sdk'/path).read_text()
        for name in names:
            if name != 'ftCo_800A0C8C':
                bodies += extract(text, name)
    (output/'cpu_original.h').write_text(bodies, encoding='utf-8')
    lifecycle = {
        'src/melee/ft/kinds/ftNess/ftness.c': ['ftNs_Init_OnDeath', 'ftNs_Init_OnDamage'],
        'src/melee/ft/kinds/ftNess/ftnessattackhi4.c': ['ftNs_AttackHi4_YoyoItemSetUnk2',
            'ftNs_AttackHi4_YoyoItemSetFlag', 'ftNs_AttackHi4_YoyoItemDespawn'],
        'src/melee/ft/kinds/ftNess/ftnessspecialn.c': ['ftNs_SpecialN_ItemPKFlushSetNULL'],
        'src/melee/ft/kinds/ftNess/ftnessspecialhi.c': ['ftNs_SpecialHi_TakeDamage'],
        'src/melee/ft/kinds/ftNess/ftnessattacks4.c': ['ftNs_AttackS4_ItemNessBatRemove'],
        'src/melee/it/kinds/itnessyoyo.c': ['it_802BE958_inline', 'it_802BE958', 'it_802BE5B8'],
        'src/melee/it/kinds/itnessbat.c': ['it_802AD6B8'],
        'src/melee/it/kinds/itnesspkflash.c': ['it_802AAA50'],
        'src/melee/it/kinds/itnesspkthunderball.c': ['it_802AB9C0'],
        'src/melee/it/kinds/itnesspkthundertrail.c': ['it_802AC5D8'],
        'src/melee/it/it_26B1.c': ['it_8026B724', 'it_8026B73C'],
    }
    (output/'lifecycle_original.h').write_text(''.join(
        extract((snapshot/'sdk'/path).read_text(), name)
        for path, names in lifecycle.items() for name in names), encoding='utf-8')
    table = declaration((snapshot/'sdk/src/melee/ft/kinds/ftNess/ftness.c').read_text(),
                        'MotionState ftNs_Init_MotionStateTable')
    clone = (snapshot/'sdk/src/melee/ft/kinds/ftNess/ftness_clone.c').read_text()
    base = ''.join(declaration(clone, marker) for marker in [
        'static const NativeFighterBone ftNessClone_Bones',
        'static const NativeFighterArticleAsset ftNessClone_ArticleAssets',
        'static const Fighter_DemoStrings ftNessClone_DemoMotionNames',
        'const NativeFighterDefinition ftNessClone_Definition'])
    callbacks = sorted(set(re.findall(r'\bftNs_\w+\b', table+base)))
    callbacks = [n for n in callbacks if n != 'ftNs_Init_MotionStateTable' and
                 not n.startswith(('ftNs_MS_', 'ftNs_MF_', 'ftNs_SM_'))]
    callbacks.append('ftCamera_UpdateCameraBox')
    (output/'base.h').write_text('\n'.join('#define '+n+' stock_slot' for n in callbacks)+
        '\n'+table+base+'\n'+'\n'.join('#undef '+n for n in callbacks)+'\n')
    required = sorted(set(re.findall(r'host->(\w+) == 0', generated)))
    (output/'required_host.h').write_text('\n'.join('host.'+name+
        '=(__typeof__(host.'+name+'))unexpected;' for name in required)+'\n')
    return {'cpu': {path: sha(snapshot/'sdk'/path) for path in originals},
            'lifecycle': {path: sha(snapshot/'sdk'/path) for path in lifecycle}}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--iso', type=Path, required=True)
    parser.add_argument('--reference', type=Path, required=True,
                        help='Immutable pre46 snapshot containing unchanged common decisions')
    parser.add_argument('--prepare-only', action='store_true')
    args = parser.parse_args()
    snapshot, output, iso = args.snapshot.resolve(), args.output.resolve(), args.iso.resolve()
    reference = args.reference.resolve()
    if any(output == p or p in output.parents for p in [snapshot, HERE]):
        raise RuntimeError('Output may not modify source or snapshot.')
    if (output / 'project').exists():
        raise RuntimeError('Use a fresh output directory.')
    for root in [snapshot, reference]:
        manifest = {p.replace('\\', '/'): h for p, h in
                    json.loads((root / 'freeze.json').read_text()).items()}
        for path, expected in manifest.items():
            if path.startswith(('sdk/', 'compiler/')) or path == 'bin/native_content_tool.exe':
                if sha(root / path) != expected:
                    raise RuntimeError('Snapshot hash mismatch: ' + str(root/path))
    candidate = compose(output)
    source_census = census(snapshot, output)
    project = output / 'project'
    tool = snapshot / 'bin/native_content_tool.exe'
    run([str(tool), 'clone-stock', str(iso), 'Ness', str(project),
         'Unregistered Ness full composition'], output / 'clone.log')
    run([str(tool), 'import-parameters', str(project), str(output / 'candidate.fms'),
         str(project / 'behavior.fms')], output / 'import.log')
    parameters = (project / 'fighter_parameters.fmp').read_text()
    receipt = {'status': 'prepared unregistered composition; source comparison pending',
               'snapshot': str(snapshot), 'snapshot_sha256': sha(snapshot / 'freeze.json'),
               'reference': str(reference), 'reference_sha256': sha(reference/'freeze.json'),
               'tool_sha256': sha(tool), 'iso_sha256': sha(iso),
               'candidate_sha256': sha(output / 'candidate.fms'),
               'imported_sha256': sha(project / 'behavior.fms'),
               'parameters_sha256': sha(project / 'fighter_parameters.fmp'),
               'census_sha256': sha(output / 'census.json'),
               'component_inputs': json.loads((output / 'component_inputs.json').read_text()),
               'candidate_bytes': len(candidate.encode()),
               'helper_count': len(re.findall(r'^(?:export )?func ', candidate, re.M)),
               'state_fields': len(re.findall(r'^state ', candidate, re.M)),
               'named_parameters': len(re.findall(r'^(?:bool|int|float) ', parameters, re.M)),
               'fixture_sources': {p.name: sha(p) for p in HERE.iterdir() if p.is_file()},
               'stock_gameplay_phases': source_census['nonnull_gameplay_phases']}
    if not args.prepare_only:
        run([str(tool), 'script-c', str(project / 'behavior.fms'), 'ness_fighter_composition',
             str(project / 'behavior.generated.c')], output / 'generate.log')
        vswhere = Path(os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)')) / \
            'Microsoft Visual Studio/Installer/vswhere.exe'
        installation = subprocess.run([str(vswhere), '-latest', '-products', '*', '-requires',
            'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'],
            capture_output=True, text=True, check=True).stdout.strip()
        setup = Path(installation) / 'VC/Auxiliary/Build/vcvars64.bat'
        clang = snapshot / 'compiler/clang.exe'
        flags = [str(clang), '-std=gnu11', '-O1', '-ffp-contract=off', '-ffunction-sections',
                 '-fdata-sections', '-DMELEE_NATIVE=1', '-D_CRT_SECURE_NO_WARNINGS',
                 '-D_USE_MATH_DEFINES', '-D_DLL', '-D_MT', '-Xclang', '--dependent-lib=msvcrt',
                 '-I' + str(snapshot / 'sdk/src'),
                 '-I' + str(snapshot / 'sdk/extern/dolphin/include')]
        command = flags + ['-c', str(project / 'behavior.generated.c'),
                           '-o', str(output / 'candidate.obj')]
        batch = output / 'compile.cmd'
        batch.write_text('@echo off\ncall "' + str(setup) + '" -vcvars_ver=14.44 >nul\n'
                         'if errorlevel 1 exit /b 1\n' + subprocess.list2cmdline(command) +
                         '\nif errorlevel 1 exit /b 1\n')
        run(['cmd.exe', '/d', '/c', str(batch)], output / 'compile.log')
        generated = (project/'behavior.generated.c').read_text()
        originals = prepare_comparisons(snapshot, output, generated)
        from common_extract import prepare
        originals.update(prepare(reference, snapshot, output))
        commands = [flags + ['-I'+str(project), '-I'+str(output), str(HERE/(name+'_compare.c')),
            str(snapshot/'sdk/src/melee/ft/native_fighter_parameters.c'),
            '-fuse-ld=lld', '-Wl,/OPT:REF', '-o', str(output/(name+'_compare.exe'))]
            for name in ['cpu','lifecycle','common','projectile','notifications']]
        batch.write_text('@echo off\ncall "'+str(setup)+'" -vcvars_ver=14.44 >nul\n'
                         'if errorlevel 1 exit /b 1\n'+''.join(subprocess.list2cmdline(command)+
                         '\nif errorlevel 1 exit /b 1\n' for command in commands))
        run(['cmd.exe', '/d', '/c', str(batch)], output/'comparisons_compile.log')
        result = run([str(output/'cpu_compare.exe'), str(project/'fighter_parameters.fmp')],
                     output/'cpu_result.log')
        match = re.search(r'Ness composed CPU: (\d+) checks, 0 failures, (\d+) source pairs', result)
        if not match or int(match[2]) != 2007:
            raise RuntimeError('CPU source comparison coverage mismatch: '+result)
        candidate_match=re.search(r'Ness candidate CPU: (\d+) pairs, (\d+) bounded threshold deltas, maximum angle delta (\S+)',result)
        if not candidate_match or (int(candidate_match[1]),int(candidate_match[2]))!=(25920,624):
            raise RuntimeError('Candidate CPU coverage mismatch: '+result)
        life_result = run([str(output/'lifecycle_compare.exe'), str(project/'fighter_parameters.fmp')],
                          output/'lifecycle_result.log')
        life = re.search(r'Ness composed lifecycle: (\d+) checks, 0 failures, (\d+) source pairs', life_result)
        if not life or int(life[2]) != 8464:
            raise RuntimeError('Lifecycle comparison coverage mismatch: '+life_result)
        expected={
            'common': (r'Ness composed common: (\d+) checks, 0 failures, (\d+) jump pairs, (\d+) presentation pairs, (\d+) constructor pairs', (792,60,36)),
            'projectile': (r'Ness composed projectile: (\d+) checks, 0 failures, (\d+) source pairs, (\d+) direct query pairs', (32400,594)),
            'notifications': (r'Ness composed notifications: (\d+) checks, 0 failures, (\d+) source pairs', (149,)),
        }
        extra = {}
        for name in ['common','projectile','notifications']:
            result = run([str(output/(name+'_compare.exe')), str(project/'fighter_parameters.fmp')],
                         output/(name+'_result.log'))
            pattern, counts=expected[name]
            coverage=re.search(pattern,result)
            if not coverage or tuple(map(int,coverage.groups()[1:]))!=counts:
                raise RuntimeError(name+' source coverage mismatch: '+result)
            extra[name] = {'checks':int(coverage[1]), 'pair_counts':counts,
                           'result':result.strip(), 'result_sha256':sha(output/(name+'_result.log'))}
        receipt.update(status='bounded full composition source acceptance; unregistered, no gameplay acceptance',
                       generated_sha256=sha(project / 'behavior.generated.c'),
                       clang_sha256=sha(clang), original_inputs=originals,
                       common_results=extra,
                       cpu_results={'checks':int(match[1]),'source_pairs':int(match[2]),
                                    'candidate_pairs':int(candidate_match[1]),
                                    'binary32_threshold_deltas':int(candidate_match[2]),
                                    'maximum_threshold_delta':float(candidate_match[3]),
                                    'result_sha256':sha(output/'cpu_result.log')},
                       lifecycle_results={'checks':int(life[1]),'source_pairs':int(life[2]),
                                          'result_sha256':sha(output/'lifecycle_result.log')})
    (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({k: receipt[k] for k in ['status', 'candidate_bytes', 'helper_count',
                                            'state_fields', 'named_parameters']}, indent=2))
    print('Receipt:', output / 'receipt.json')


if __name__ == '__main__':
    main()
