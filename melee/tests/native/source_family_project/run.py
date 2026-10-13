"""Actual source family authoring/CLI checks; no game or runtime publication."""
import argparse, hashlib, json, re, subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'tool', 'iso', 'draft', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if output.drive.upper() != 'A:' or output.exists():
        parser.error('Use a fresh private output directory on A:')
    output.mkdir(parents=True)
    sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    inputs = {str(p.resolve()): sha(p) for p in args.draft.rglob('*') if p.is_file()}
    inputs[str(args.exe.resolve())] = sha(args.exe)
    inputs[str(args.tool.resolve())] = sha(args.tool)
    commands = []

    def run(label, command, expected=0):
        command = list(map(str, command))
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True,
                                timeout=180, creationflags=subprocess.CREATE_NO_WINDOW)
        (output / (label + '.log')).write_text(result.stdout + result.stderr)
        commands.append({'label': label, 'command': command, 'exit': result.returncode})
        if result.returncode != expected:
            raise RuntimeError(label + ':\n' + result.stdout + result.stderr)
        return result

    fixture = run('fixture', [args.exe, args.iso, args.draft, output / 'drafts'])
    summary = json.loads(fixture.stdout)
    cache = output / 'cache'
    ids = []
    for mode in range(3):
        draft = output / 'drafts' / ('mode-' + str(mode))
        package = run('package-' + str(mode), [args.tool, '--root', cache,
            'package-project', draft, draft / 'portrait.png'])
        identity = re.search(r'Cached source fighter ([0-9a-f]{64})', package.stdout).group(1)
        ids.append(identity)
        verify = run('verify-' + str(mode), [args.tool, '--root', cache, 'verify-package', identity])
        assert ('1 members' if mode == 0 else '2 members') in verify.stdout
        for member in range(1 if mode == 0 else 2):
            path = draft if member == 0 else draft / 'partner'
            run(f'check-{mode}-{member}', [args.tool, 'source-check', path, path / 'behavior.fms'])

    # Transport validation cannot establish source semantics. Preserve the
    # canonical byte extents while changing only the second member's script.
    packet = bytearray((cache / 'cache' / (ids[1] + '.fmc')).read_bytes())
    portrait_size = int.from_bytes(packet[8:12], 'little')
    cursor = 208 + portrait_size
    script_offset = script_bytes = 0
    for member in range(2):
        directory = 80 + member * 64
        for chunk in range(8):
            size = int.from_bytes(packet[directory + 32 + chunk*4:directory + 36 + chunk*4], 'little')
            if member == 1 and chunk == 4:
                script_offset, script_bytes = cursor, size
            cursor += size
    assert cursor == len(packet)
    invalid = b'fighter:\n    behavior independent\naction Main:\n    on load:\n        unavailable_operation()\n'
    assert len(invalid) <= script_bytes
    packet[script_offset:script_offset+script_bytes] = invalid + b' ' * (script_bytes-len(invalid))
    bad_id = hashlib.sha256(packet).hexdigest()
    (cache / 'cache' / (bad_id + '.fmc')).write_bytes(packet)
    bad = run('reject-second-member', [args.tool, '--root', cache, 'verify-package', bad_id], expected=1)
    assert 'member 1 rejected' in bad.stderr

    private_source = output / 'drafts' / 'mode-1' / 'partner' / 'behavior.fms'
    original = private_source.read_bytes()
    before_cache = sorted(p.name for p in (cache / 'cache').iterdir())
    try:
        private_source.write_bytes(invalid)
        run('reject-draft-package', [args.tool, '--root', cache, 'package-project',
            output / 'drafts' / 'mode-1', output / 'drafts' / 'mode-1' / 'portrait.png'], expected=1)
        assert before_cache == sorted(p.name for p in (cache / 'cache').iterdir())
    finally:
        private_source.write_bytes(original)
    assert all(sha(Path(path)) == value for path, value in inputs.items())
    receipt = {'status': 'PASS source family authoring and CLI', 'fixture': summary,
        'commands': commands, 'inputs': inputs, 'package_ids': ids,
        'scope': 'Actual DAT/resource/member preparation and project/package roundtrip; stock paired asset authoring; per-member source checks; verification rejects bad second source and failed builds leave cache unchanged.',
        'limits': ['Complete Zelda/Sheik and Popo/Nana scripts not accepted here.',
                   'Runtime family publication, actor identity, gameplay, graphics and rollback not exercised.']}
    (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({'status': receipt['status'], 'fixture': summary,
                      'receipt': str(output / 'receipt.json')}))

if __name__ == '__main__':
    main()
