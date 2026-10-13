"""Compare complete Ice Shot/Blizzard source bodies with generated callbacks.

This console runner never starts Melee or builds the shared repository.
"""
import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('root', 'baseline', 'output', 'iso'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    root, base, out, iso = [p.resolve() for p in
                           (args.root, args.baseline, args.output, args.iso)]
    if base.name == 'baseline65' or not (base / 'freeze.json').is_file():
        raise ValueError('An accepted frozen SDK66 or newer is required.')
    sdk, compiler, tool = base / 'sdk', base / 'compiler/clang.exe', base / 'bin/native_content_tool.exe'
    for required in (compiler, tool, iso):
        if not required.is_file():
            raise FileNotFoundError(required)
    out.mkdir(parents=True, exist_ok=False)
    commands, results = [], {}

    def run(command, folder, name):
        command = list(map(str, command))
        commands.append(command)
        result = subprocess.run(command, cwd=root, capture_output=True,
                                text=True, encoding='utf-8', errors='replace')
        (folder / name).write_text(result.stdout + result.stderr, encoding='utf-8')
        if result.returncode:
            raise RuntimeError(str(folder / name) + '\n' + (result.stdout + result.stderr)[-8000:])
        return result.stdout

    for form in ('popo', 'nana'):
        folder = out / form
        folder.mkdir()
        run([sys.executable, HERE / 'fixture.py', '--root', root,
             '--baseline', base, '--output', folder, '--iso', iso,
             '--form', form], folder, 'extract.log')
        run([tool, 'script-c', folder / 'owned.fms', 'ice_climbers_' + form,
             folder / 'generated.inc'], folder, 'generate.log')
        generation = (folder / 'generated.inc').read_text(encoding='utf-8')
        provenance = json.loads((folder / 'provenance.json').read_text(encoding='utf-8'))
        parameters = provenance['parameters']
        match = re.search(r'count != (\d+) \|\| schema != (0x[0-9a-fA-F]+)ULL', generation)
        if not match or int(match[1]) != len(parameters):
            raise ValueError('Generated parameter schema does not match named staging import.')
        (folder / 'parameters.inc').write_text(
            'static const uint32_t parameter_bits[] = {' +
            ','.join('UINT32_C(%d)' % p['bits'] for p in parameters) + '};\n' +
            'static const uint64_t parameter_schema = ' + match[2] + 'ULL;\n',
            encoding='utf-8')
        hosts = sorted(set(re.findall(r'host->(\w+) == 0', generation)))
        (folder / 'hosts.inc').write_text('\n'.join(
            'host.' + h + ' = (__typeof__(host.' + h + '))unexpected;' for h in hosts) + '\n',
            encoding='utf-8')
        flags = [compiler, '-std=gnu11', '-O2', '-ffp-contract=off',
                 '-fms-runtime-lib=dll', '-DMELEE_NATIVE=1', '-D_DLL', '-D_MT',
                 '-D_USE_MATH_DEFINES', '-D_CRT_SECURE_NO_WARNINGS',
                 '-DFORM_NANA=' + str(int(form == 'nana')),
                 '-I' + str(sdk / 'src'), '-I' + str(sdk / 'extern/dolphin/include'),
                 '-I' + str(folder)]

        def compile(command, name):
            batch = folder / (name + '.cmd')
            batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/'
                             'VC/Auxiliary/Build/vcvars64.bat" >nul\n' +
                             subprocess.list2cmdline(list(map(str, command))) + '\n', encoding='utf-8')
            run(['cmd', '/c', batch], folder, name + '.log')

        compile(flags + ['-shared', '-x', 'c', folder / 'generated.inc',
                         '-o', folder / 'module.dll'], 'compile_module')
        compile(flags + [HERE / 'test.c', sdk / 'src/melee/ft/native_callback_lifetime.c',
                         '-o', folder / 'test.exe'], 'compile_test')
        result = run([folder / 'test.exe'], folder, 'run.log')
        counts = re.search(r'PASS (\d+) original/generated pairs, (\d+) checks, (\d+) authored safety cases', result)
        if not counts:
            raise ValueError('Missing complete console result.')
        results[form] = dict(pairs=int(counts[1]), checks=int(counts[2]),
                             authored_safety_cases=int(counts[3]), provenance=provenance)
        print(form + ': ' + result.strip(), flush=True)

    archive = out / 'source_snapshot'
    archive.mkdir()
    for path in HERE.iterdir():
        if path.is_file():
            shutil.copyfile(path, archive / path.name)
    inputs = {p for p in sdk.rglob('*') if p.is_file()}
    inputs.update(p for p in (base / 'compiler').rglob('*') if p.is_file())
    inputs.update(p for p in HERE.iterdir() if p.is_file())
    inputs.update([tool, iso, base / 'freeze.json', root / 'tools/extract_native_animation.py'])
    receipt = dict(status='PASS unregistered Ice Shot/Blizzard composition',
                   snapshot=str(base), results=results, commands=commands,
                   inputs={str(p): sha(p) for p in sorted(inputs)},
                   outputs={str(p): sha(p) for p in sorted(out.rglob('*')) if p.is_file()},
                   limits=[
                       'Retail big-endian named staging parameters; registered IC clone/import and paired package are not implemented.',
                       'Four fighter states, accessory cues and both complete article state machines; full Squall/Belay cleanup excluded.',
                       'Motion/animation, allocation/model construction, stage sweeps and floor geometry response, particle rendering and debug bookkeeping are controlled engine boundaries.',
                       'Actual shared falling/facing/scaling/wall/ceiling/reflection bodies and actual lifetime registry execute in the console fixture.',
                       'Only projected owned Ice/Blizzard lifecycle fields are compared; unrelated stock private-union fields are outside the component.',
                       'Source invalidation cases are authored safety checks, separate from healthy original-pointer comparisons.',
                       'No gameplay, registration, installation, paired AI foundation or independent fighter acceptance.'])
    (out / 'result.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    print(out / 'result.json')


if __name__ == '__main__':
    main()
