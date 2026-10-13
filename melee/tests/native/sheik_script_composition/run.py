"""Frozen console-only Sheik source/generated comparisons; no gameplay."""
import argparse, hashlib, json, re, subprocess, sys, shutil
from pathlib import Path
HERE = Path(__file__).resolve().parent
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('root', 'baseline', 'output'): parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--iso', type=Path)
    args = parser.parse_args(); root = args.root.resolve(); base = args.baseline.resolve(); out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False); sdk = base / 'sdk'; compiler = base / 'compiler/clang.exe'; tool = base / 'bin/native_content_tool.exe'
    iso = args.iso.resolve() if args.iso else root.parent / 'Super Smash Bros. Melee (USA) (v1.02).iso'; commands = []
    def run(command, name):
        command = list(map(str, command)); commands.append(command)
        result = subprocess.run(command, cwd=root, capture_output=True, text=True)
        (out / name).write_text(result.stdout + result.stderr)
        if result.returncode: raise RuntimeError(str(out / name) + '\n' + (result.stdout + result.stderr)[-7000:])
        return result.stdout
    run([sys.executable, HERE / 'fixture.py', '--root', root, '--sdk', sdk, '--iso', iso, '--output', out], 'extract.log')
    run([tool, 'script-c', out / 'owned.fms', 'sheik_source', out / 'generated.inc'], 'generate.log')
    generation = (out / 'generated.inc').read_text(); provenance = json.loads((out / 'provenance.json').read_text())['parameters']
    count, schema = re.search(r'count != (\d+) \|\| schema != (0x[0-9a-fA-F]+)ULL', generation).groups(); assert int(count) == len(provenance)
    (out / 'parameters.inc').write_text('static const uint32_t parameter_bits[] = {' + ','.join('UINT32_C(%d)' % p['bits'] for p in provenance) + '};\nstatic const uint64_t parameter_schema = ' + schema + 'ULL;\nstatic const char* parameter_names[] = {' + ','.join(json.dumps(p['name']) for p in provenance) + '};\n')
    hosts = sorted(set(re.findall(r'host->(\w+) == 0', generation)))
    (out / 'hosts.inc').write_text('\n'.join('host.' + h + ' = (__typeof__(host.' + h + '))unexpected_boundary;' for h in hosts) + '\n')
    (out / 'attributes.inc').write_text('\n'.join('{ uint32_t bits = UINT32_C(%d); memcpy((uint8_t*)&sk_attrs + %d, &bits, 4); }' % (p['bits'], p['offset']) for p in provenance) + '\n')
    flags = [compiler, '-std=gnu11', '-O2', '-fms-runtime-lib=dll', '-DMELEE_NATIVE=1', '-D_DLL', '-D_MT', '-D_USE_MATH_DEFINES', '-D_CRT_SECURE_NO_WARNINGS', '-I' + str(sdk / 'src'), '-I' + str(sdk / 'extern/dolphin/include'), '-I' + str(out)]
    def compile(command, name):
        batch = out / (name + '.cmd')
        batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" >nul\n' + subprocess.list2cmdline(list(map(str, command))) + '\n')
        run(['cmd', '/c', batch], name + '.log')
    compile(flags + ['-shared', '-x', 'c', out / 'generated.inc', '-o', out / 'module.dll'], 'compile_module')
    compile(flags + [HERE / 'test.c', sdk / 'src/melee/ft/native_callback_lifetime.c', '-o', out / 'test.exe'], 'compile_test')
    result = run([out / 'test.exe'], 'run.log'); counts = re.search(r'PASS (\d+) original/generated pairs, (\d+) checks, (\d+) authored safety cases', result)
    if not counts: raise ValueError('Missing result counts')
    inputs = {p for p in sdk.rglob('*') if p.is_file()}; inputs.update(p for p in (base / 'compiler').rglob('*') if p.is_file())
    inputs.update(p for p in HERE.rglob('*') if p.is_file() and '__pycache__' not in p.parts); inputs.update([tool, base / 'freeze.json', iso, root / 'tools/extract_native_animation.py'])
    archive = out / 'source_snapshot'; archive.mkdir()
    for path in HERE.iterdir():
        if path.is_file(): shutil.copyfile(path, archive / path.name)
    receipt = {'status': 'PASS unregistered Sheik Vanish standalone source composition', 'snapshot': base.name,
        'pairs': int(counts[1]), 'checks': int(counts[2]), 'authored_safety_cases': int(counts[3]),
        'provenance': json.loads((out / 'provenance.json').read_text()), 'commands': commands,
        'inputs': {str(p): sha(p) for p in sorted(inputs)}, 'outputs': {str(p): sha(p) for p in sorted(out.iterdir()) if p.is_file()},
        'limits': ['Vanish355..360 entries/all24phases/accessories and complete Vanish initializer/animation/damage/interact policy only.',
        'Neutral and Chain are pending separate source acceptance; transformation361..364 and paired package/CPU requests excluded.',
        'No registered clone/imported package, independent activation, installed module or gameplay acceptance.',
        'Actual shared drift/fall/deceleration/facing/force-air/ground-to-self/angle/expiry bodies and native_callback_lifetime execute directly.',
        'Allocation/model/particle/stage/motion/animation boundaries controlled and recorded in order. Original SpawnItem prefix is compared with generated constructor arguments; real allocator/model construction is not executed.',
        'Finite named floats compare exact bits, IDs/timers remain Int; no padding/union hash comparison or removed gameplay fields.']}
    (out / 'result.json').write_text(json.dumps(receipt, indent=2) + '\n'); print(result.strip()); print(out / 'result.json')

if __name__ == '__main__': main()
