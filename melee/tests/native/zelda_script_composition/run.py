"""Isolated source/generated Zelda comparisons; no native build or gameplay."""
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
        if result.returncode: raise RuntimeError(str(out / name) + '\n' + (result.stdout + result.stderr)[-6000:])
        return result.stdout
    run([sys.executable, HERE / 'fixture.py', '--root', root, '--sdk', sdk, '--iso', iso, '--output', out], 'extract.log')
    run([tool, 'script-c', out / 'owned.fms', 'zelda_source', out / 'generated.inc'], 'generate.log')
    generation = (out / 'generated.inc').read_text(); provenance = json.loads((out / 'provenance.json').read_text())['parameters']
    count, schema = re.search(r'count != (\d+) \|\| schema != (0x[0-9a-fA-F]+)ULL', generation).groups()
    assert int(count) == len(provenance)
    (out / 'parameters.inc').write_text('static const uint32_t parameter_bits[] = {' + ','.join('UINT32_C(%d)' % p['bits'] for p in provenance) + '};\nstatic const uint64_t parameter_schema = ' + schema + 'ULL;\n')
    hosts = sorted(set(re.findall(r'host->(\w+) == 0', generation)))
    (out / 'hosts.inc').write_text('\n'.join('host.' + h + ' = (__typeof__(host.' + h + '))unexpected_boundary;' for h in hosts) + '\n')
    assignments = []
    for p in provenance:
        destination = 'zd_attrs' if p['article_slot'] is None else ('din_attrs' if p['article_slot'] == 0 else 'explosion_attrs')
        if p['type'] == 'bool': assignments.append(f'((uint8_t*)&{destination})[{p["offset"]}] = {p["bits"]};')
        else: assignments.append('{ uint32_t bits = UINT32_C(%d); memcpy((uint8_t*)&%s + %d, &bits, 4); }' % (p['bits'], destination, p['offset']))
    (out / 'attributes.inc').write_text('\n'.join(assignments) + '\n')
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
    inputs.update(p for p in HERE.rglob('*') if p.is_file()); inputs.update([tool, base / 'freeze.json', iso, root / 'tools/extract_native_animation.py'])
    archive = out / 'source_snapshot'; archive.mkdir()
    for path in HERE.iterdir():
        if path.is_file(): shutil.copyfile(path, archive / path.name)
    receipt = {'status': 'PASS unregistered partial Zelda phase composition', 'snapshot': base.name,
        'pairs': int(counts[1]), 'checks': int(counts[2]), 'authored_safety_cases': int(counts[3]), 'provenance': json.loads((out / 'provenance.json').read_text()), 'commands': commands,
        'inputs': {str(p): sha(p) for p in sorted(inputs)}, 'outputs': {str(p): sha(p) for p in sorted(out.iterdir()) if p.is_file()},
        'limits': ['No clone registration, normal project import, installed package, independent activation or gameplay.',
          'Neutral complete entry/phases/accessory; Din/Explosion full initializer decisions, state phases, secondary callbacks and cleanup; item allocator/constructor SpawnItem prefix is controlled, not compared.',
          'Animation/motion entry, stage probes, particle rendering and explosion allocation are controlled boundaries; source and generated decisions execute directly.',
          'Explosion on_created is compared directly to its real initializer including immediate animation, hitbox damage/radius, expiry tick, rumble/quake and debug order. Whole allocation/model construction is not claimed.',
          'Din same-address creator retirement is outside original valid-pointer parity; generated source generation safety is separate.',
          'Side-B fighter phases, teleport, transformation, paired CSS/package coupling, CPU and debug dress toggle remain pending.']}
    (out / 'result.json').write_text(json.dumps(receipt, indent=2) + '\n'); print(result.strip()); print(out / 'result.json')
if __name__ == '__main__': main()
