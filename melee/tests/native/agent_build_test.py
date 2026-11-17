"""Build, diagnose and export complete editable families using the shipped API."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

tool, iso = map(lambda value: str(Path(value).resolve()), sys.argv[1:])
windows = Path(os.environ['SystemRoot'])

def snapshot(root):
    return {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in root.rglob('*') if p.is_file()}

with tempfile.TemporaryDirectory(prefix='Agent build with spaces ') as temporary:
    root = Path(temporary)
    (root / 'userdata').mkdir()
    env = dict(os.environ, LOCALAPPDATA=str(root / 'userdata'), PATH=str(windows / 'System32') + ';' + str(windows))
    def run(*arguments):
        return subprocess.run([tool, *map(str, arguments)], capture_output=True, text=True, env=env,
                              timeout=90, creationflags=subprocess.CREATE_NO_WINDOW)
    def call(name, args, expected=True):
        result = run('agent', 'call', name, json.dumps(args))
        assert result.stdout, (name, result.returncode, result.stderr)
        data = json.loads(result.stdout)
        assert data['ok'] == expected, (name, data, result.stderr)
        return data['result'] if expected else data['error']
    for fighter in ('Mario', 'Zelda'):
        project = root / fighter
        cloned = run('clone-stock', iso, fighter, project, fighter)
        assert cloned.returncode == 0, cloned.stderr
        if fighter == 'Zelda':
            cloned = run('clone-stock', iso, 'Sheik', project / 'partner', 'Sheik')
            assert cloned.returncode == 0, cloned.stderr
            (project / 'family.txt').write_text('mode=alternate\nprimary_key=main\npartner_key=partner\npartner=partner\n')
        args = {'project': str(project)}
        info = call('project.inspect', args)
        members = [row['key'] for row in info['members']]
        assert len(members) == (2 if fighter == 'Zelda' else 1)
        originals = {}
        for member in members:
            selected = dict(args, member=member)
            listed = call('scripts.list', selected)
            file = listed['files'][0]['file']
            source = call('scripts.read', dict(selected, file=file))['text']
            originals[member] = (file, source)
            call('scripts.update', dict(selected, revision=listed['revision'], files=[{'file': file, 'text': source}]))
        unchanged = snapshot(project)
        revision = call('project.inspect', args)['revision']
        built = call('project.build', dict(args, revision=revision))
        assert built['accepted'] and built['cached'] and not built['saved'] and not built['installed'], built
        assert len(built['member_reports']) == len(members)
        assert all(row['accepted'] for row in built['member_reports'])
        assert snapshot(project) == unchanged
        exported = root / (fighter + '.fmc')
        output = call('packages.export', {'package_id': built['package_id'], 'output': str(exported)})
        assert output['bytes'] == built['package_bytes']
        assert hashlib.sha256(exported.read_bytes()).hexdigest() == built['package_id']
        original = exported.read_bytes()
        assert call('packages.export', {'package_id': built['package_id'], 'output': str(exported)}, False)['code'] == 'export_unavailable'
        assert exported.read_bytes() == original
        refused = root / 'not-a-package.txt'
        assert call('packages.export', {'package_id': built['package_id'], 'output': str(refused)}, False)['code'] == 'invalid_output'
        assert not refused.exists()
        # Both members' independent errors are returned in one completed build.
        for member in members:
            file, source = originals[member]
            selected = dict(args, member=member)
            revision = call('project.inspect', selected)['revision']
            call('scripts.update', dict(selected, revision=revision, files=[{'file': file, 'text': source +
                '\nfunc BadOne():\n    missing.one()\n\nfunc BadTwo():\n    missing.two()\n'}]))
        broken_snapshot = snapshot(project)
        revision = call('project.inspect', args)['revision']
        bad = call('project.build', dict(args, revision=revision))
        assert not bad['accepted'] and bad['diagnostic_count'] >= 2 * len(members), bad
        assert all(not row['accepted'] for row in bad['member_reports'])
        assert snapshot(project) == broken_snapshot
        assert call('project.build', dict(args, revision='0' * 64), False)['code'] == 'revision_conflict'
print('PASS: complete editable-family builds, batch diagnostics, unchanged drafts, exact export and preserved destinations')
