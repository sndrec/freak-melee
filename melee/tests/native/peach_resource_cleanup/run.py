"""Generate and run the unregistered cleanup composition against one frozen SDK."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shlex
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--baseline', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root = args.root.resolve()
    baseline = args.baseline.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    here = Path(__file__).resolve().parent
    sdk = baseline / 'sdk'
    commands = []

    def run(command, log):
        commands.append(command)
        result = subprocess.run(command, cwd=root, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, text=True)
        (output / log).write_text(result.stdout)
        if result.returncode:
            print(result.stdout, file=sys.stderr)
            raise SystemExit(result.returncode)
        return result.stdout

    run([str(baseline / 'bin/native_content_tool.exe'), 'script-c',
         str(here / 'candidate.fms'), 'peach_resource_cleanup',
         str(output / 'generated.inc')], 'generate.log')
    run([sys.executable, str(here / 'fixture.py'), '--root', str(sdk),
         '--output', str(output)], 'extract.log')
    units = ['src/melee/it/native_item_owner.c',
             'src/melee/ft/native_callback_lifetime.c',
             'src/melee/ft/native_fighter_events.c']
    compile_args = [str(baseline / 'compiler/clang.exe'), '-std=gnu11', '-O2',
                    '-finstrument-functions', '-fms-runtime-lib=dll',
                    '-DMELEE_NATIVE=1', '-D_USE_MATH_DEFINES',
                    '-D_CRT_SECURE_NO_WARNINGS', '-D_DLL', '-D_MT',
                    '-I' + str(sdk / 'src'),
                    '-I' + str(sdk / 'extern/dolphin/include'), '-I' + str(output),
                    '-MMD', '-MF', str(output / 'dependencies.d'),
                    str(here / 'test.c'), *[str(sdk / path) for path in units],
                    '-o', str(output / 'test.exe')]
    vcvars = Path('C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat')
    batch = output / 'compile.cmd'
    batch.write_text('@echo off\ncall "' + str(vcvars) + '" >nul\n' +
                     subprocess.list2cmdline(compile_args) + '\n')
    run(['cmd', '/c', str(batch)], 'compile.log')
    stdout = run([str(output / 'test.exe')], 'run.log')
    counts = re.search(r'(\d+) actual/generated pairs, (\d+) safety cases, (\d+) checks', stdout)
    if not counts:
        raise SystemExit('Missing fixture result')
    sources = json.loads((output / 'source.json').read_text())
    inputs = {here / name for name in ['run.py', 'fixture.py', 'test.c',
                                      'candidate.fms', 'README.md']}
    inputs.add(here.parent / 'peach_turnip_composition_fixture.py')
    inputs.update(sdk / path for path in sources['sources'])
    inputs.update(sdk / path for path in units)
    # Hash all SDK files: Clang -MF with multiple units retains only the last
    # unit's dependency set. Full frozen inputs avoid an incomplete header claim.
    inputs.update(path for path in sdk.rglob('*') if path.is_file())
    inputs.update([baseline / 'freeze.json', baseline / 'bin/native_content_tool.exe',
                   baseline / 'compiler/clang.exe'])
    inputs.update(output / name for name in ['generated.inc', 'actual.inc',
                  'source.json', 'generate.log', 'extract.log', 'compile.cmd',
                  'compile.log', 'run.log', 'dependencies.d', 'test.exe'])
    receipt = {'status': 'PASS source-only bounded cleanup composition',
               'snapshot': baseline.name, 'pairs': int(counts[1]),
               'safety_cases': int(counts[2]), 'checks': int(counts[3]),
               'body_count': sources['body_count'], 'statement_slice_count': 1,
               'commands': commands,
               'inputs': {str(path): hashlib.sha256(path.read_bytes()).hexdigest()
                          for path in sorted(inputs)}}
    (output / 'result.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(stdout.strip())
    print('Receipt:', output / 'result.json')


if __name__ == '__main__':
    main()
