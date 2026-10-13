"""Direct native pose routing/lifetime fixture; selected assets/math are controlled."""
import argparse, hashlib, json, re, subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]

def body(text, name):
    match = re.search(r'^[\w \t*]+\b' + re.escape(name) + r'\s*\([^;{}]*\)\s*\{', text, re.M)
    if not match:
        raise RuntimeError('missing production function ' + name)
    at = text.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (text[at] == '{') - (text[at] == '}')
        at += 1
    return text[match.start():at]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--compiler', type=Path)
    parser.add_argument('--extract-only', action='store_true')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    if not args.extract_only and (not args.compiler or out.drive.upper() != 'A:' or out.exists()):
        parser.error('fresh private A: output required')
    out.mkdir(parents=True, exist_ok=args.extract_only)
    catalog_path = ROOT / 'src/melee/ft/native_script_bindings.c'
    guards_path = ROOT / 'src/melee/ft/native_script_binding_properties.c'
    catalog, guards = catalog_path.read_text(), guards_path.read_text()
    first = catalog.index('#define F ')
    last = catalog.index('\n};', catalog.index('static const NativeScriptBindingDescriptor catalog')) + 3
    extracted = catalog[first:last] + '\n' + '#include <melee/ft/native_script_binding_item_resources.h>\n#include <melee/ft/native_script_binding_spawn.h>\n'+body(catalog, 'nativeScriptBindings_Descriptor')
    for name in ['error', 'nativeScriptBindings_SourceAlive', 'nativeScriptBindings_SourceFighterLayout']:
        extracted += '\n' + re.sub(r'\berror\b', 'pose_guard_error', body(guards, name))
    (out / 'source_contract.inc').write_text(extracted + '\n')
    if args.extract_only:
        return
    units = [ROOT/'src/melee/ft/native_script_binding_item_resources_catalog.c', ROOT/'src/melee/ft/native_script_binding_spawn_catalog.c', HERE / 'test.c', ROOT / 'src/melee/ft/native_script_binding_pose.c',
             ROOT / 'src/melee/ft/native_callback_lifetime.c', ROOT / 'src/melee/gm/native_match_pool.c']
    compiler = args.compiler.resolve()
    inputs = units + [catalog_path, guards_path, HERE / 'run.py', compiler]
    inputs += list((ROOT / 'src').rglob('*.h')) + list((ROOT / 'extern/dolphin/include').rglob('*.h'))
    sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    hashes = {str(path): sha(path) for path in inputs}
    command = [str(compiler), '-std=gnu11', '-O2', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
               '-Wno-array-parameter', '-Wno-missing-field-initializers',
               '-DMELEE_NATIVE=1', '-D_CRT_SECURE_NO_WARNINGS', '-D_USE_MATH_DEFINES',
               '-I' + str(ROOT / 'src'), '-I' + str(ROOT / 'extern/dolphin/include'), '-I' + str(out)]
    command += list(map(str, units)) + ['-o', str(out / 'test.exe')]
    commands = []
    for stage, cmd in [('compile', command), ('fixture', [str(out / 'test.exe')])]:
        result = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, timeout=120,
                                creationflags=subprocess.CREATE_NO_WINDOW)
        (out / (stage + '.log')).write_text(result.stdout + result.stderr)
        commands.append({'stage': stage, 'command': cmd, 'exit': result.returncode})
        (out / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n')
        if result.returncode:
            raise RuntimeError(stage + ' failed:\n' + (result.stdout + result.stderr)[-8000:])
    if any(sha(Path(path)) != expected for path, expected in hashes.items()):
        raise RuntimeError('input changed during fixture')
    receipt = {'status': 'PASS', 'summary': json.loads(result.stdout), 'commands': commands,
               'input_sha256': hashes,
               'boundaries': ['production pose dispatcher, actual catalog and source data/actor guards',
                              'actual generational match pool and physical callback lifetime',
                              'selected assets/views, topology validator and shared pose calls controlled'],
               'limits': ['no animation math, retail topology, renderer or full Sheik gameplay acceptance']}
    (out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt['summary']))

if __name__ == '__main__':
    main()
