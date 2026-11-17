"""Create, import and rename complete drafts using the shipped authoring tools."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def snapshot(root):
    return {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in root.rglob('*') if p.is_file()}


def main():
    tool, iso = (Path(p).resolve() for p in sys.argv[1:3])
    with tempfile.TemporaryDirectory(prefix='Agent draft creation ') as temporary:
        root = Path(temporary)
        appdata = root / 'userdata'
        appdata.mkdir()
        windows = Path(os.environ['SystemRoot'])
        env = dict(os.environ, LOCALAPPDATA=str(appdata), PATH=str(windows / 'System32') + ';' + str(windows))
        def run(*parts, executable=tool):
            return subprocess.run([str(executable), *map(str, parts)], cwd=root, env=env,
                capture_output=True, encoding='utf-8', timeout=90, creationflags=subprocess.CREATE_NO_WINDOW)
        def call(method, args=None, expected=True, executable=tool):
            result = run('agent', 'call', method, json.dumps(args or {}), executable=executable)
            assert result.stdout, (method, result.returncode, result.stderr)
            reply = json.loads(result.stdout)
            assert reply['ok'] == expected, (method, reply, result.stderr)
            return reply['result'] if expected else reply['error']
        stock = call('stock.list', {'limit': 200})
        choices = {row['name']: row for row in stock['fighters']}
        assert 'Kirby' not in choices and all(row['template_available'] for row in choices.values())
        assert choices['Zelda']['mode'] == 'alternate' and choices['Ice Climbers']['mode'] == 'companion'
        first = call('stock.list', {'limit': 3})
        assert len(first['fighters']) == 3 and first['next_offset'] == 3
        assert call('stock.list', {'query': 'ganondorf'})['fighters'][0]['name'] == 'Ganondorf'
        assert not call('fighters.installed')['fighters']
        location = appdata / 'Freak Melee'
        location.mkdir(exist_ok=True)
        (location / 'iso-path.txt').write_text(str(iso) + '\n')
        projects = {}
        for name in ('Mario', 'Zelda', 'Ice Climbers'):
            arguments = {'fighter_id': choices[name]['fighter_id'], 'title': name + ' Agent'}
            if name == 'Zelda': arguments.update(iso=str(iso), output=str(root / 'Chosen Zelda'))
            created = call('project.clone_stock', arguments)
            project = Path(created['project'])
            projects[name] = project
            assert created['saved'] and not created['installed'] and Path(created['agent_guide']).is_file()
            assert len(created['members']) == (1 if name == 'Mario' else 2)
            for member in created['members']:
                path = Path(member['path'])
                assert (path / 'fighter_state.fms').is_file() and list((path / 'moves').glob('*.fms'))
                assert (path / 'fighter_parameters.fmp').is_file() and (path / 'resources.fmr').is_file()
                assert call('scripts.check', {'project': str(project), 'member': member['key']})['accepted']
            assert (project / 'portrait.png').is_file() and not (project / 'creation.pending').exists()
        assert len(call('projects.list')['projects']) == 2, 'Explicit non-library output should not appear in the local library'
        mario = projects['Mario']
        before = snapshot(mario)
        info = call('project.inspect', {'project': str(mario)})
        renamed = call('project.rename', {'project': str(mario), 'revision': info['revision'], 'title': 'Agent Comet'})
        assert renamed['changed'] and renamed['saved'] and not renamed['installed']
        after = snapshot(mario)
        assert {key for key in before if before[key] != after[key]} == {'project.txt'}
        same = call('project.rename', {'project': str(mario), 'revision': renamed['revision'], 'title': 'Agent Comet'})
        assert not same['changed'] and same['revision'] == renamed['revision']
        assert call('project.rename', {'project': str(mario), 'revision': info['revision'], 'title': 'Stale'}, False)['code'] == 'revision_conflict'
        assert call('project.rename', {'project': str(mario), 'revision': renamed['revision'], 'title': 'bad\nname'}, False)['code'] == 'invalid_title'
        assert snapshot(mario) == after
        existing = root / 'existing'
        existing.mkdir()
        (existing / 'sentinel').write_text('preserve')
        call('project.clone_stock', {'fighter_id': choices['Mario']['fighter_id'], 'title': 'Blocked', 'output': str(existing)}, False)
        assert snapshot(existing) == {'sentinel': hashlib.sha256(b'preserve').hexdigest()}
        call('project.clone_stock', {'fighter_id': 255, 'title': 'Unsupported'}, False)
        bad_package = root / 'bad.fmc'
        bad_package.write_text('this is not a fighter')
        call('project.import', {'package': str(bad_package), 'output': str(root / 'bad-import')}, False)
        assert not (root / 'bad-import').exists()
        for name in ('Mario', 'Zelda'):
            project = projects[name]
            args = {'project': str(project)}
            baseline = snapshot(project)
            built = call('project.build', dict(args, revision=call('project.inspect', args)['revision']))
            assert built['accepted'] and not built['installed'] and snapshot(project) == baseline
            package = root / (name + '.fmc')
            call('packages.export', {'package_id': built['package_id'], 'output': str(package)})
            assert hashlib.sha256(package.read_bytes()).hexdigest() == built['package_id']
            imported = call('project.import', {'package': str(package)})
            assert len(imported['members']) == len(built['member_reports'])
            imported_path = Path(imported['project'])
            assert not (imported_path / 'installed.id').exists()
            assert call('scripts.check', {'project': str(imported_path)})['accepted']
            tuning = call('attributes.list', dict(args, query='gravity'))
            gravity = next(row['value'] for row in tuning['attributes'] if row['name'] == 'gravity')
            altered = call('attributes.edit', dict(args, revision=tuning['revision'], changes=[{'name': 'gravity', 'value': gravity + .01}]))
            altered_files = snapshot(project)
            rejected = call('project.install', dict(args, revision=altered['revision'], package_id=built['package_id']), False)
            assert rejected['code'] == 'build_not_accepted' and snapshot(project) == altered_files
            call('attributes.edit', dict(args, revision=altered['revision'], changes=[{'name': 'gravity', 'value': gravity}]))
            installed_build = call('project.install', dict(args, revision=call('project.inspect', args)['revision'], package_id=built['package_id']))
            assert installed_build['accepted'] and installed_build['saved'] and installed_build['installed']
            baseline = snapshot(project)
            installed = call('fighters.installed', {'query': built['package_id']})['fighters']
            assert len(installed) == 1 and installed[0]['member_count'] == len(imported['members'])
            copied = call('project.clone_installed', {'package_id': built['package_id'], 'title': name + ' Copied'})
            assert copied['project'] != str(project) and not (Path(copied['project']) / 'installed.id').exists()
            assert len(copied['members']) == len(imported['members']) and snapshot(project) == baseline
            for member in copied['members']:
                assert call('scripts.check', {'project': copied['project'], 'member': member['key']})['accepted']
        # An incomplete installation cannot publish a half-created draft.
        broken = root / 'incomplete installation'
        (broken / 'ui').mkdir(parents=True)
        shutil.copy2(tool, broken / tool.name)
        for library in tool.parent.glob('*.dll'): shutil.copy2(library, broken / library.name)
        shutil.copy2(tool.parent / 'ui/mario_full.fms', broken / 'ui/mario_full.fms')
        failed = root / 'unfinished'
        error = call('project.clone_stock', {'fighter_id': choices['Mario']['fighter_id'], 'title': 'Missing images', 'output': str(failed)}, False, executable=broken / tool.name)
        assert error['code'] == 'clone_failed' and not failed.exists()
        print('PASS: complete stock families, owned split sources/images, independent package import/clone, protected rename and failed-publication preservation')


if __name__ == '__main__': main()
