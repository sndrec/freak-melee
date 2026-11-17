"""Edit common attributes atomically through the packaged native API."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def snapshot(root):
    return {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in root.rglob('*') if p.is_file()}


def main():
    tool, iso = (Path(p).resolve() for p in sys.argv[1:3])
    with tempfile.TemporaryDirectory(prefix='Agent attributes ') as temporary:
        root = Path(temporary)
        (root / 'userdata').mkdir()
        windows = Path(os.environ['SystemRoot'])
        env = dict(os.environ, LOCALAPPDATA=str(root / 'userdata'),
                   PATH=str(windows / 'System32') + ';' + str(windows))
        def run(*parts):
            return subprocess.run([str(tool), *map(str, parts)], env=env, cwd=root,
                capture_output=True, encoding='utf-8', timeout=60, creationflags=subprocess.CREATE_NO_WINDOW)
        def call(method, args, expected=True):
            result = run('agent', 'call', method, json.dumps(args))
            assert result.stdout, (result.returncode, result.stderr)
            reply = json.loads(result.stdout)
            assert reply['ok'] == expected, (method, reply)
            return reply['result'] if expected else reply['error']
        project = root / 'Zelda family'
        for fighter, path in [('Zelda', project), ('Sheik', project / 'partner')]:
            result = run('clone-stock', iso, fighter, path, fighter)
            assert result.returncode == 0, (result.stdout, result.stderr)
        (project / 'family.txt').write_text('mode=alternate\nprimary_key=main\npartner_key=partner\npartner=partner\n')
        target = {'project': str(project)}
        listed = call('attributes.list', dict(target, limit=200))
        fields = {row['name']: row for row in listed['attributes']}
        assert listed['total'] == 97 and listed['next_offset'] is None
        assert fields['weight']['exclusive_minimum'] and fields['weight']['minimum'] == 0
        assert fields['max_jumps']['type'] == 'integer' and fields['max_jumps']['minimum'] == 1
        assert fields['weight_independent_throws_mask']['maximum'] == 255
        page = call('attributes.list', dict(target, limit=4))
        assert page['next_offset'] == 4 and len(page['attributes']) == 4
        assert call('attributes.list', dict(target, query='jumps and air physics'))['total'] > 10
        before = snapshot(project)
        changed = call('attributes.edit', dict(target, revision=listed['revision'], changes=[
            {'name': 'gravity', 'value': .123456789}, {'name': 'max_jumps', 'value': 3},
            {'name': 'weight_independent_throws_mask', 'value': 15}]))
        assert changed['changed'] and changed['saved'] and not changed['installed']
        assert changed['changes'][0]['requested'] != changed['changes'][0]['applied']
        after = snapshot(project)
        assert {key for key in before if before[key] != after[key]} == {'fighter.dat'}
        same = call('attributes.edit', dict(target, revision=changed['revision'], changes=[
            {'name': 'gravity', 'value': changed['changes'][0]['applied']}]))
        assert not same['changed'] and same['revision'] == changed['revision']
        for bad in [
            [{'name': 'weight', 'value': 0}], [{'name': 'model_scaling', 'value': 1e-100}],
            [{'name': 'max_jumps', 'value': 1.5}], [{'name': 'weight_independent_throws_mask', 'value': 256}],
            [{'name': 'gravity', 'value': 1e40}], [{'name': 'missing', 'value': 1}],
            [{'name': 'gravity', 'value': 1}, {'name': 'gravity', 'value': 2}],
        ]:
            call('attributes.edit', dict(target, revision=changed['revision'], changes=[
                {'name': 'walk_max_vel', 'value': 200}, *bad]), False)
            assert snapshot(project) == after
        stale = call('attributes.edit', dict(target, revision=listed['revision'], changes=[
            {'name': 'weight', 'value': 1}]), False)
        assert stale['code'] == 'revision_conflict'
        partner = call('attributes.edit', dict(target, member='partner', revision=changed['revision'], changes=[
            {'name': 'weight', 'value': 150}]))
        paired = snapshot(project)
        assert {key for key in paired if paired[key] != after[key]} == {'partner/fighter.dat'}
        assert partner['revision'] != changed['revision']
        assert next(row['value'] for row in call('attributes.list', target)['attributes'] if row['name'] == 'weight') == fields['weight']['value']
        print('PASS: attribute metadata, native limits, atomic grouped edits, quantization, stale revisions and family-member isolation')


if __name__ == '__main__': main()
