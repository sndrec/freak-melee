"""Compare explicit-flag hitboxes with the previous native retail evaluator."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BASELINE = '6876936'


def body(text, name):
    match = re.search(r'^[\w \t*]+\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{', text, re.M)
    if not match:
        raise ValueError('Missing function '+name)
    at = text.index('{', match.start())+1
    depth = 1
    while depth:
        depth += (text[at] == '{')-(text[at] == '}')
        at += 1
    return text[match.start():at]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    paths = [ROOT/'src/melee/ft'/name for name in
             ['ftaction.c', 'ftaction_data.c', 'ftcoll.c', 'ftcommon.c', 'native_hitbox_lifetime.c']]
    paths.append(ROOT/'src/melee/lb/lbcollision.c')
    action, data, collision, common, lifetime, history = [p.read_text() for p in paths]
    old = subprocess.run(['git', 'show', BASELINE+':melee/src/melee/ft/ftaction.c'],
                         cwd=ROOT.parent, capture_output=True, text=True, check=True,
                         creationflags=subprocess.CREATE_NO_WINDOW).stdout
    pieces = [data[data.index('typedef struct FieldLayout'):data.index('bool ftActionData_Import(')],
              body(history, 'lbColl_80008440'), body(history, 'lbColl_CopyHitCapsule'),
              body(collision, 'ftColl_800768A0'), body(common, 'ftCommon_80080484'),
              body(lifetime, 'nativeHitboxLifetime_Clear'), body(action, 'ftAction_800715EC'),
              body(old, 'ftAction_8007121C')
              .replace('void ftAction_8007121C(', 'static void original_hitbox(', 1)
              .replace('ftColl_8007AD18(fp, hitbox)', 'ftColl_8007AD18(fp, cmd->u->create_hitbox_0.id)'),
              body(action, 'ftAction_8007121C')]
    generated = out/'bodies.inc'
    generated.write_text('\n\n'.join(pieces), encoding='utf-8')
    inputs = paths+[HERE/'test.c', Path(__file__).resolve(), generated, args.compiler.resolve()]
    inputs += list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
    sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    hashes = {str(p): sha(p) for p in inputs}
    flags = ['-std=gnu11', '-O2', '-ffp-contract=off', '-DMELEE_NATIVE=1',
             '-D_CRT_SECURE_NO_WARNINGS', '-D_USE_MATH_DEFINES',
             '-I'+str(ROOT/'src'), '-I'+str(ROOT/'extern/dolphin/include'), '-I'+str(out)]
    commands = []
    exe = out/'test.exe'
    for stage, command in [
        ('strict', [args.compiler, *flags, '-Wall', '-Wextra', '-Werror', '-Wno-array-parameter',
                    '-Wno-unused-function', '-Wno-unused-variable', '-Wno-unused-parameter', '-fsyntax-only', HERE/'test.c']),
        ('compile', [args.compiler, *flags, HERE/'test.c', '-o', exe]), ('fixture', [exe])]:
        run = subprocess.run(list(map(str, command)), cwd=ROOT, capture_output=True, text=True,
                             timeout=180, creationflags=subprocess.CREATE_NO_WINDOW)
        (out/(stage+'.log')).write_text(run.stdout+run.stderr, encoding='utf-8')
        commands.append(dict(stage=stage, exit=run.returncode, command=list(map(str, command))))
        (out/'commands.json').write_text(json.dumps(commands, indent=2))
        if run.returncode:
            raise RuntimeError(stage+': '+(run.stdout+run.stderr)[-6000:])
    if any(sha(Path(p)) != digest for p, digest in hashes.items()):
        raise RuntimeError('Inputs changed during review')
    summary = json.loads(run.stdout.splitlines()[-1])
    receipt = dict(passed=True, summary=summary, baseline=BASELINE, input_sha256=hashes,
                   commands=commands,
                   actual=['current and previous native hitbox handlers, skip and source-bit decoding',
                           'actual group/victim-history operations, lifetime clear and smash timer update'],
                   controlled=['preconstructed Fighter/parts/joints and semantic bone lookup',
                               'damage and angle leaves; no invoked render proc'],
                   limits=['not full gameplay, staleness/scaling, geometry evaluation or rollback acceptance'])
    (out/'receipt.json').write_text(json.dumps(receipt, indent=2), encoding='utf-8')
    print(json.dumps(summary))


if __name__ == '__main__':
    main()
