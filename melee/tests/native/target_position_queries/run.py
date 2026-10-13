"""Actual same-author translation and original/common nearest query; console only."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def body(text, name):
    match = re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b' +
                      re.escape(name) + r'\s*\([^;{}]*\)\s*\{', text)
    if not match:
        raise ValueError('Missing function ' + name)
    depth, end = 1, match.end()
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--source-root', type=Path, default=ROOT)
    args = parser.parse_args()
    snapshot, output, root = args.snapshot.resolve(), args.output.resolve(), args.source_root.resolve()
    if output.drive.upper() != 'A:' or output.exists():
        parser.error('Use a fresh output directory on A:')
    frozen = json.loads((snapshot/'freeze.json').read_text())
    def verify():
        for path, expected in frozen.items():
            if sha(snapshot/path) != expected:
                raise RuntimeError('Changed frozen input '+path)
    verify()
    output.mkdir(parents=True)
    sdk = snapshot/'sdk'
    paths = ['src/melee/it/native_article_refs.c', 'src/melee/it/native_article_refs.h',
             'src/melee/ft/kinds/ftCommon/ftCo_0A01.c', 'src/melee/ft/kinds/ftCommon/ftCo_0A01.h',
             'src/melee/ft/ftcpuattack.c']
    inputs = {str(root/p):sha(root/p) for p in paths}
    inputs.update({str(p):sha(p) for p in HERE.iterdir() if p.is_file()})
    original_path = sdk/paths[2]
    old, new = original_path.read_text(), (root/paths[2]).read_text()
    common = ['ftCo_IsAlly', 'ftCo_IsAlly_dontinline', 'inlineD0',
              'ftCo_800A1C44', 'ftCo_800A0F00', 'inlineD1']
    pieces = []
    for name in common:
        unchanged = body(old, name)
        assert unchanged == body(new, name), 'Changed original eligibility '+name
        pieces.append(unchanged)
    library = (sdk/'src/melee/ft/ftlib.c').read_text()
    pieces.append(body(library, 'ftLib_8008732C'))
    pieces.append(body(old, 'ftCo_800A4E8C'))
    identity = new[new.index('typedef struct CpuQueryIdentity'):new.index('\n#endif', new.index('typedef struct CpuQueryIdentity'))]
    pieces += [identity, body(new, 'ftCo_CpuNearestEligibleTarget')]
    (output/'actual_bodies.h').write_text('\n\n'.join(pieces)+'\n', newline='\n')
    overlay = output/'overlay/src/melee/it'
    overlay.mkdir(parents=True)
    (overlay/'native_article_refs.h').write_bytes((root/paths[1]).read_bytes())
    (output/'native_article_refs.c').write_bytes((root/paths[0]).read_bytes())
    command = [snapshot/'compiler/clang.exe', '-std=gnu11', '-O2', '-ffp-contract=off',
        '-fuse-ld=lld', '-DMELEE_NATIVE=1', '-D_USE_MATH_DEFINES', '-D_CRT_SECURE_NO_WARNINGS',
        '-I'+str(output/'overlay/src'), '-I'+str(output), '-I'+str(overlay),
        '-I'+str(sdk/'src'), '-I'+str(sdk/'src/melee/it'), '-I'+str(sdk/'extern/dolphin/include'),
        HERE/'test.c', output/'native_article_refs.c', sdk/'src/melee/ft/native_callback_lifetime.c',
        '-o', output/'test.exe']
    commands = []
    for stage, cmd in [('compile', command), ('run', [output/'test.exe'])]:
        cmd = list(map(str, cmd))
        result = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, timeout=120)
        (output/(stage+'.log')).write_text(result.stdout+result.stderr, newline='\n')
        commands.append({'stage':stage, 'command':cmd, 'exit':result.returncode})
        if result.returncode:
            raise RuntimeError(stage+' failed:\n'+result.stdout+result.stderr)
    verify()
    for path, expected in inputs.items():
        if sha(Path(path)) != expected:
            raise RuntimeError('Changed live input '+path)
    receipt = {'status':'PASS actual query bodies and original nearest comparison',
        'result':json.loads(result.stdout), 'snapshot_sha256':sha(snapshot/'freeze.json'),
        'inputs':inputs, 'original_source_sha256':sha(original_path),
        'extracted_sha256':sha(output/'actual_bodies.h'), 'commands':commands,
        'boundaries':['Actual complete article refs and registry lifetime production units execute.',
            'Original nearest body and unchanged original eligibility helpers execute alongside the directly refactored body.',
            'Shared game-mode, stage-bound, special-state and Definition queries are controlled deterministic dependencies; no game renderer or full CPU scheduler.',
            'Fixture injects invalidation at controlled dependency boundaries to check rejection; those pure engine observations do not normally dispatch authored callbacks.',
            'Primary root local translation is distinct from Item.pos and world position. No speculative world-query API added.']}
    (output/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n', newline='\n')
    print(json.dumps(receipt['result']))


if __name__ == '__main__':
    main()
