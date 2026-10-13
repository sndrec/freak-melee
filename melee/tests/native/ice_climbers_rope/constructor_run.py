"""Run only the primary Belay source console comparison with a matching frozen SDK."""
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
    for key in ('root', 'baseline', 'reference', 'output', 'iso'):
        parser.add_argument('--' + key, type=Path, required=True)
    x = parser.parse_args()
    root, baseline, reference, out = x.root.resolve(), x.baseline.resolve(), x.reference.resolve(), x.output.resolve()
    if not (baseline / 'freeze.json').is_file() or not (reference / 'freeze.json').is_file():
        raise ValueError('Matching candidate and immutable original SDK manifests required')
    out.mkdir(parents=True, exist_ok=False)
    commands = []
    def run(command, name):
        command = list(map(str, command));commands.append(command)
        result = subprocess.run(command, cwd=root, capture_output=True, text=True, encoding='utf-8', errors='replace')
        (out / name).write_text(result.stdout + result.stderr, encoding='utf-8')
        (out / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n', encoding='utf-8')
        if result.returncode:
            raise RuntimeError(str(out / name) + '\n' + (result.stdout + result.stderr)[-10000:])
        return result.stdout
    run([sys.executable, HERE / 'constructor_fixture.py', '--root', root, '--baseline', baseline,
         '--reference', reference, '--output', out, '--iso', x.iso], 'extract.log')
    tool, clang, sdk = baseline / 'bin/native_content_tool.exe', baseline / 'compiler/clang.exe', baseline / 'sdk'
    provenance = json.loads((out / 'provenance.json').read_text(encoding='utf-8'))
    schema, counts, bit_arrays, hosts = [], [], [], set()
    flags = [clang, '-std=gnu11', '-O2', '-ffp-contract=off', '-fms-runtime-lib=dll', '-DMELEE_NATIVE=1',
             '-D_DLL', '-D_MT', '-D_USE_MATH_DEFINES', '-D_CRT_SECURE_NO_WARNINGS', '-I'+str(sdk / 'src'),
             '-I'+str(sdk / 'extern/dolphin/include'), '-I'+str(out)]
    def compile(command, name):
        batch = out / (name + '.cmd')
        batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" >nul\n' +
                         subprocess.list2cmdline(list(map(str, command))) + '\n', encoding='utf-8')
        run(['cmd', '/c', batch], name + '.log')
    for index, form in enumerate(('popo', 'nana')):
        target = out / form
        run([tool, 'script-c', target / 'owned.fms', 'ice_linked_' + form, target / 'generated.c'], form + '-generate.log')
        text = (target / 'generated.c').read_text(encoding='utf-8')
        match = re.search(r'count != (\d+) \|\| schema != (0x[0-9a-fA-F]+)ULL', text)
        parameters = provenance['forms'][form]['parameters']
        if not match or int(match[1]) != len(parameters):
            raise ValueError('Parameter schema mismatch: ' + form)
        counts.append(len(parameters));schema.append(match[2]+'ULL')
        bit_arrays.append('static const uint32_t bits_%d[]={%s};' % (index, ','.join('UINT32_C(%d)' % p['bits'] for p in parameters)))
        hosts.update(re.findall(r'host->(\w+) == 0', text))
        compile(flags + ['-shared', target / 'generated.c', '-o', target / 'module.dll'], form + '-module')
    (out / 'paired_parameters.inc').write_text('\n'.join(bit_arrays) + '\nstatic const uint32_t*parameter_bits[]={bits_0,bits_1};\n' +
        'static const uint32_t parameter_count[]={%s};\n' % ','.join(map(str,counts)) +
        'static const uint64_t parameter_schema[]={%s};\n' % ','.join(schema), encoding='utf-8')
    (out / 'paired_hosts.inc').write_text('\n'.join('host.%s=(__typeof__(host.%s))unexpected;' % (h,h) for h in sorted(hosts)) + '\n', encoding='utf-8')
    compile(flags + [HERE / 'constructor_test.c', sdk / 'src/melee/ft/native_callback_lifetime.c',
                    sdk / 'src/melee/ft/native_fighter_relations.c', sdk/'src/melee/it/native_article_resource.c', sdk/'src/melee/it/native_article_refs.c', '-o', out / 'test.exe'], 'console')
    result = run([out / 'test.exe', out / 'popo/module.dll', out / 'nana/module.dll'], 'run.log')
    match = re.search(r'PASS (\d+) original/generated pairs, (\d+) checks, (\d+) authored safety cases', result)
    if not match:
        raise ValueError('Missing console acceptance receipt')
    inputs = {}
    snapshot = out / 'namespace';snapshot.mkdir()
    for path in (HERE/n for n in ('constructor_fixture.py','constructor_run.py','constructor_test.c','constructor_cases.inc','constructor_dependencies.inc','primary_fixture.py','prepare.py','primary.fms','rope.fms')):
        if path.is_file():
            shutil.copyfile(path, snapshot / path.name);inputs[str(path.relative_to(root))] = sha(path)
    for borrowed in ('ice_climbers_linked','ice_climbers_composition'):
        for path in (HERE.parent / borrowed).iterdir():
            if path.is_file():inputs[str(path.relative_to(root))] = sha(path)
    for folder in (sdk, baseline / 'compiler'):
        for path in folder.rglob('*'):
            if path.is_file():inputs[str(path)] = sha(path)
    for path in (tool, baseline / 'freeze.json', reference / 'freeze.json'):
        inputs[str(path)] = sha(path)
    for name in ('extract_native_animation.py', 'prepare_native_assets.py'):
        path = root / 'tools' / name
        inputs[str(path.relative_to(root))] = sha(path)
    for path in provenance['sources']:
        if not path.startswith('candidate:'):inputs[str(reference / 'sdk' / path)] = sha(reference / 'sdk' / path)
    receipt = dict(pairs=int(match[1]), checks=int(match[2]), authored_safety_cases=int(match[3]), commands=commands,
        baseline=str(baseline), reference=str(reference), input_hashes=inputs, provenance=provenance,
        scope=provenance['scope'], outputs={str(p.relative_to(out)):sha(p) for p in out.rglob('*') if p.is_file()})
    (out / 'result.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    print(result.strip())

if __name__ == '__main__':
    main()
