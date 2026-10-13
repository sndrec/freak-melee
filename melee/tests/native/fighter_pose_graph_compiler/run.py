"""Console-only candidate compiler and actual generated fighter pose operations."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root, out, clang = args.root.resolve(), args.output.resolve(), args.compiler.resolve()
    out.mkdir(parents=True, exist_ok=False)
    here = Path(__file__).resolve().parent
    source_list = re.search(r'set\(NATIVE_FIGHTER_SCRIPT_SOURCES\s+(.*?)\)',
                            (root/'CMakeLists.txt').read_text(), re.S).group(1)
    units = [root/name for name in source_list.split()]
    pose_unit = root/'tools/native_fighter_script_pose.c'
    if pose_unit not in units:
        units.append(pose_unit)
    units += [root/'src/melee/ft/native_fighter_parameters.c', here/'compiler.c']
    inputs = units + list((root/'tools').glob('*.h'))
    inputs += list((root/'src').rglob('*.h')) + list((root/'extern/dolphin/include').rglob('*.h'))
    inputs += [clang, here/'probe.fms', here/'module.c', here/'run.py']
    hashes = {str(path): sha(path) for path in inputs}
    common = [str(clang), '-std=gnu11', '-O2', '-fuse-ld=lld', '-DMELEE_NATIVE=1',
              '-D_CRT_SECURE_NO_WARNINGS', '-D_USE_MATH_DEFINES',
              '-I'+str(root/'src'), '-I'+str(root/'extern/dolphin/include')]
    commands, results = [], {}
    def run(name, command):
        command=list(map(str, command)); commands.append(command)
        proc=subprocess.run(command, cwd=root, text=True, capture_output=True)
        (out/(name+'.log')).write_text(proc.stdout+proc.stderr)
        if proc.returncode:
            raise RuntimeError(name+' failed:\n'+(proc.stdout+proc.stderr)[-12000:])
        results[name]=proc.stdout.strip()
    compiler=out/'compiler.exe'
    run('compile-compiler', common+list(map(str, units))+['-o',compiler])
    for form in ('action', 'state'): (out/form).mkdir()
    action, state = out/'action/pose_probe.inc', out/'state/pose_probe.inc'
    run('generate', [compiler, here/'probe.fms', action, state])
    for form in ('action', 'state'):
        exe=out/form/'module.exe'
        run('compile-'+form, common+['-I'+str(out/form),here/'module.c','-o',exe])
        run('run-'+form,[exe])
    for path, digest in hashes.items():
        if sha(Path(path)) != digest: raise RuntimeError('Input changed during fixture: '+path)
    receipt={'scope':'Current candidate source/header inputs, explicit compiler; no frozen SDK claim, game or actual engine traversal proof.',
             'inputs':hashes,'commands':commands,'results':results,
             'generated_sha256':{form:sha(out/form/'pose_probe.inc') for form in ('action','state')}}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({'receipt':str(out/'receipt.json'),'results':results}))


if __name__=='__main__':
    main()
