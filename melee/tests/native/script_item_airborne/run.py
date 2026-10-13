"""Actual typed Item airborne assignment, with controlled actor publication."""
import argparse, hashlib, importlib.util, json, subprocess, sys
from pathlib import Path
sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
OLD = HERE.parent / 'script_item'
spec = importlib.util.spec_from_file_location('item_extract', OLD / 'extract_item.py')
extractor = importlib.util.module_from_spec(spec)
spec.loader.exec_module(extractor)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    if out.exists():
        parser.error('Use a fresh directory on A:')
    out.mkdir(parents=True)
    inputs = extractor.extract(ROOT, out / 'script_item_actual.h')
    boundary = OLD / 'unavailable_families.c'
    prefix = boundary.read_text().split('// This primitive-only Item fixture')[0]
    (out / 'unrelated.c').write_text(prefix, newline='\n')
    names = ['binding_item', 'bindings', 'binding_item_resources_catalog', 'binding_spawn_catalog', 'binding_properties', 'constants', 'value', 'math', 'expression', 'source', 'statement', 'program', 'bytecode', 'vm', 'storage']
    units = [ROOT / 'src/melee/ft' / ('native_script_' + name + '.c') for name in names]
    units += [ROOT / 'src/melee/ft/native_callback_lifetime.c', ROOT / 'src/melee/gm/native_match_pool.c', ROOT / 'src/melee/ft/native_script_binding_effect_resolve.c', OLD / 'engine_fixture.c', out / 'unrelated.c', HERE / 'test.c', HERE.parent / 'script_source_fighter_absent.c']
    absent = {'item', 'kinetics', 'state', 'combat', 'setup', 'presentation', 'transition'}
    units += [p for p in HERE.parent.glob('script_bindings_unavailable_*.c') if p.stem.removeprefix('script_bindings_unavailable_') not in absent]
    def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
    inputs.update({str(p): sha(p) for p in units})
    for p in [boundary, OLD / 'extract_item.py', HERE / 'run.py', ROOT / 'src/melee/it/native_item_lifetime.h', ROOT / 'src/melee/ft/native_script_bindings.h', ROOT / 'src/melee/it/native_script_item_actor.h', ROOT / 'tools/native_fighter_item_script.c', ROOT / 'src/melee/it/kinds/itgamewatchpanic.c']:
        inputs[str(p)] = sha(p)
    exe = out / 'test.exe'
    flags = [args.compiler, '-std=c11', '-O2', '-ffp-contract=off', '-fuse-ld=lld', '-Wall', '-Wextra', '-Werror', '-Wno-array-parameter', '-Wno-missing-field-initializers', '-DMELEE_NATIVE=1', '-D_USE_MATH_DEFINES', '-D_CRT_SECURE_NO_WARNINGS', '-I' + str(ROOT / 'src'), '-I' + str(ROOT / 'extern/dolphin/include'), '-I' + str(out)]
    commands = []
    for stage, command in [('compile', flags + units + ['-o', exe]), ('run', [exe])]:
        command = list(map(str, command))
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=120, creationflags=subprocess.CREATE_NO_WINDOW)
        (out / (stage + '.log')).write_text(result.stdout + result.stderr, newline='\n')
        commands.append({'stage': stage, 'command': command, 'exit': result.returncode})
        if result.returncode: raise RuntimeError(stage + ' failed:\n' + result.stdout + result.stderr)
    for path, expected in inputs.items():
        if sha(Path(path)) != expected: raise RuntimeError('Input changed: ' + path)
    receipt = {'status': 'PASS', 'result': json.loads(result.stdout), 'inputs': inputs, 'commands': commands,
               'scope': 'Production typed source/index/body/bytecode/VM, real pool/registry/storage, outer property dispatcher and Item backend. Whole Item field comparisons against direct ground_or_air assignment; Workshop holdable rejection and source/schema/lifetime negatives.',
               'limits': 'Actor spec/view/status publication is controlled. Assets, constructor, scheduler, native Panic picked-up animation transition and live gameplay are not exercised. Unrelated engine families explicitly reject.'}
    (out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n', newline='\n')
    print(json.dumps(receipt['result']))

if __name__ == '__main__': main()
