"""Exercise the shipped native JSON/MCP entry points with an isolated real draft."""
import hashlib
import asyncio
import json
import os
import shutil
from pathlib import Path
import subprocess
import sys
import tempfile


async def launcher_exchange(command, env, initialize, inspect):
    process = await asyncio.create_subprocess_exec(*command, env=env,
        stdin=asyncio.subprocess.PIPE, stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.PIPE, creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        # Keep stdin open while waiting for each response. A transport that
        # buffers requests until EOF would otherwise pass a batch-only check.
        process.stdin.write((initialize + '\n').encode())
        await process.stdin.drain()
        first = json.loads(await asyncio.wait_for(process.stdout.readline(), 10))
        process.stdin.write((inspect + '\n').encode())
        await process.stdin.drain()
        second = json.loads(await asyncio.wait_for(process.stdout.readline(), 10))
        process.stdin.close()
        await asyncio.wait_for(process.wait(), 10)
        stderr = (await process.stderr.read()).decode()
        assert process.returncode == 0, stderr
        assert await process.stdout.read() == b''
        return [first, second]
    finally:
        if process.returncode is None:
            subprocess.run(['taskkill.exe', '/PID', str(process.pid), '/T', '/F'],
                capture_output=True, creationflags=subprocess.CREATE_NO_WINDOW)
            await asyncio.wait_for(process.wait(), 10)


def main():
    tool, iso = map(lambda value: Path(value).resolve(), sys.argv[1:3])
    with tempfile.TemporaryDirectory(prefix='native-agent-', dir=tool.parent) as temporary:
        folder = Path(temporary)
        (folder / 'appdata').mkdir()
        windows = Path(os.environ['SystemRoot'])
        env = dict(os.environ, LOCALAPPDATA=str(folder / 'appdata'),
                   PATH=str(windows/'System32') + ';' + str(windows))

        def run(*args, expected=0, input=None):
            result = subprocess.run([str(tool), *map(str, args)], input=input,
                                    cwd=folder, env=env, capture_output=True,
                                    text=True, encoding='utf-8', timeout=90,
                                    creationflags=subprocess.CREATE_NO_WINDOW)
            assert result.returncode == expected, (args, result.returncode, result.stdout, result.stderr)
            return result.stdout

        def call(name, args=None, expected=0):
            result = json.loads(run('agent', 'call', name, json.dumps(args or {}), expected=expected))
            assert result['ok'] == (expected == 0), result
            return result['result'] if result['ok'] else result['error']

        tools = json.loads(run('agent', 'list'))
        names = {item['name'] for item in tools}
        assert {'environment.describe', 'project.inspect', 'scripts.list', 'actions.list', 'docs.search'} <= names
        assert all(item['inputSchema']['additionalProperties'] is False for item in tools)
        mutations = {'scripts.update', 'project.build', 'packages.export', 'session.build', 'session.install',
                     'session.preview.refresh', 'session.practice.start', 'session.practice.stop',
                     'session.open', 'session.member.select', 'session.workspace.select', 'session.scripts.update',
                     'session.scripts.select', 'session.save', 'session.undo', 'session.redo', 'session.disconnect'}
        assert all(item['annotations']['readOnlyHint'] == (item['name'] not in mutations) for item in tools)
        assert call('environment.describe')['installation'] == str(tool.parent)
        assert call('projects.list')['total'] == 0
        assert call('unknown', expected=1)['code'] == 'unknown_tool'
        assert call('docs.search', {'limit': 0}, expected=1)['code'] == 'invalid_arguments'
        assert call('docs.search', {'limit': -1}, expected=1)['code'] == 'invalid_arguments'
        assert call('docs.search', {'limit': 1.5}, expected=1)['code'] == 'invalid_arguments'
        assert call('docs.search', {'root': 'anything'}, expected=1)['code'] == 'invalid_arguments'
        assert call('project.inspect', expected=1)['code'] == 'invalid_arguments'
        assert call('docs.read', {'page': '../account-session.bin'}, expected=1)['code'] == 'unknown_document'
        docs = call('docs.search', {'query': 'hitbox', 'limit': 2})
        assert len(docs['pages']) == 2 and docs['total'] > 2
        page = docs['pages'][0]['page']
        assert call('docs.read', {'page': page})['text']
        bundled = call('examples.list', {'query': 'mario', 'limit': 1})
        assert len(bundled['examples']) == 1 and bundled['total'] >= 2
        assert 'fighter:' in call('examples.read', {'example': 'mario_full.fms'})['text']
        assert call('examples.read', {'example': 'fox_falco_blaster.fms'})['text']
        assert call('examples.read', {'example': '../private.fms'}, expected=1)['code'] == 'unknown_example'

        draft = folder / 'appdata/Freak Melee/projects/Agent Mario'
        run('clone-stock', iso, 'Mario', draft, 'Agent Mario')
        assert call('projects.list')['total'] == 1
        args = {'project': str(draft)}
        before = {p.relative_to(draft): hashlib.sha256(p.read_bytes()).hexdigest()
                  for p in draft.rglob('*') if p.is_file()}
        inspected = call('project.inspect', args)
        assert inspected['family_mode'] == 'single'
        assert inspected['member'] == 'main'
        assert inspected['members'][0]['title'] == 'Agent Mario'
        assert call('project.inspect', dict(args, member='missing'), expected=1)['code'] == 'unknown_member'
        files = call('scripts.list', args)
        assert files['source_origin'] == 'stock_template' and len(files['files']) >= 5
        text = call('scripts.read', dict(args, file=files['files'][0]['file']))
        assert text['text'] and text['editable_source_sha256'] == files['editable_source_sha256']
        assert call('scripts.read', dict(args, file='../other.fms'), expected=1)['code'] == 'unknown_source'
        aerials = call('actions.list', dict(args, query='AttackAir', limit=3))
        assert len(aerials['actions']) == 3 and aerials['next_offset'] == 3
        rest = call('actions.list', dict(args, query='AttackAir', offset=3))
        assert set(a['id'] for a in aerials['actions']).isdisjoint(a['id'] for a in rest['actions'])
        attrs = call('attributes.list', dict(args, limit=200))
        assert attrs['total'] == len(attrs['attributes']) == 97
        assert all('value' in field and field['description'] for field in attrs['attributes'])
        checked = call('scripts.check', args)
        assert checked['accepted'], checked
        assert checked['metadata_complete'] and checked['bodies_complete']
        assert checked['statistics']['functions'] > 0 and checked['statistics']['instructions'] > 0
        assert checked['diagnostics'] == [] and checked['editable_source_sha256'] == files['editable_source_sha256']
        after = {p.relative_to(draft): hashlib.sha256(p.read_bytes()).hexdigest()
                 for p in draft.rglob('*') if p.is_file()}
        assert before == after, 'Read-only operations changed draft files'

        # Imported defaults must preserve explicit tuning. A failed import must
        # leave the original files available for inspection and repair.
        parameters = draft / 'fighter_parameters.fmp'
        parameters.write_text('# player tuning\nint cpu_capture_first_attempt = 12\n')
        tuned = call('parameters.read', args)
        assert tuned['valid'] and tuned['text'].replace('\r\n', '\n').startswith('# player tuning\n'), tuned
        assert next(f['value'] for f in tuned['fields'] if f['name'] == 'cpu_capture_first_attempt') == 12
        assert call('scripts.check', args)['accepted']
        parameters.write_text('bool cpu_capture_first_attempt = true\n')
        broken = call('scripts.list', args)
        assert broken['parameter_import_error']
        assert call('scripts.read', dict(args, file=broken['files'][0]['file']))['text']
        rejected = call('scripts.check', args)
        assert not rejected['accepted'] and rejected['diagnostics'][0]['phase'] == 'parameter_import'
        parameters.unlink()

        # Check the same editable split-file format used by the Workshop and
        # collect independent body errors with original file/line locations.
        editable = {file['file'].replace('\\', '/'): call('scripts.read', dict(args, file=file['file']))['text']
                    for file in files['files']}
        for relative, text in editable.items():
            path = draft / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text, encoding='utf-8')
        assert call('scripts.check', args)['accepted']
        combined = ''
        for text in editable.values():
            if combined and not combined.endswith('\n'): combined += '\n'
            combined += text
        combined_path = folder / 'combined.fms'
        combined_path.write_bytes(combined.encode())
        audit = [json.loads(line) for line in run('source-audit', draft, combined_path).splitlines()]
        assert audit[-1]['package_accepted'] and audit[-1]['diagnostics'] == 0
        assert 'imports' in audit[0]
        run('script-build', draft, combined_path)
        accepted_source = (draft / 'behavior.fms').read_bytes()
        assert call('scripts.list', args)['matches_validated_source']
        state = draft / 'fighter_state.fms'
        original = state.read_text(encoding='utf-8')
        damaged = original + '\nfunc AgentBadOne():\n    missing.first()\n\nfunc AgentBadTwo():\n    missing.second()\n'
        state.write_text(damaged, encoding='utf-8')
        rejected = call('scripts.check', args)
        assert not rejected['accepted'] and rejected['diagnostic_count'] >= 2, rejected
        expected_lines = {damaged.splitlines().index('    missing.first()') + 1,
                          damaged.splitlines().index('    missing.second()') + 1}
        actual_lines = {d['line'] for d in rejected['diagnostics'] if d['file'] == 'fighter_state.fms'}
        assert expected_lines <= actual_lines, rejected
        combined_path.write_text(combined + '\nfunc AgentBad():\n    missing.call()\n', encoding='utf-8')
        run('script-build', draft, combined_path, expected=1)
        assert (draft / 'behavior.fms').read_bytes() == accepted_source
        state.write_text(original, encoding='utf-8')
        assert call('scripts.check', args)['accepted']
        for relative in editable:
            (draft / relative).unlink()

        # Saved move files are authoritative over the validated package text.
        (draft / 'fighter_state.fms').write_text('# editable state\n', encoding='utf-8')
        (draft / 'moves').mkdir(exist_ok=True)
        (draft / 'moves/002_Second.fms').write_text('# second\n', encoding='utf-8')
        (draft / 'moves/001_First.fms').write_text('# first\n', encoding='utf-8')
        files = call('scripts.list', args)
        assert files['source_origin'] == 'separate_files'
        assert [f['name'] for f in files['files']] == ['Fighter state', 'First', 'Second']
        expected = '# editable state\n# first\n# second\n'
        assert files['editable_source_sha256'] == hashlib.sha256(expected.encode()).hexdigest()
        assert not files['matches_validated_source']
        (draft / 'moves/001_First.fms').write_bytes(b'invalid\0source')
        assert call('scripts.list', args, expected=1)['code'] == 'invalid_source_set'

        def rpc(method, params=None, id=1):
            result = {'jsonrpc': '2.0', 'method': method}
            if id is not None: result['id'] = id
            if params is not None: result['params'] = params
            return json.dumps(result)

        requests = [
            rpc('tools/list', id=0),
            rpc('initialize', {'protocolVersion': '2025-11-25', 'capabilities': {},
                               'clientInfo': {'name': 'native-agent-test', 'version': 'test'}}, id='init'),
            rpc('notifications/initialized', id=None),
            rpc('tools/list', id=2),
            rpc('tools/call', {'name': 'environment.describe', 'arguments': {}}, id=3),
            rpc('tools/call', {'name': 'docs.search', 'arguments': {'limit': 0}}, id=4),
            rpc('resources/list', id=5),
            rpc('resources/read', {'uri': 'freak-melee://docs/index.md'}, id=6),
            rpc('resources/read', {'uri': 'freak-melee://docs/../account-session.bin'}, id=7),
            rpc('unknown-method', id=8),
            '{invalid',
            'x' * (4 * 1024 * 1024 + 1),
            rpc('ping', id=9),
            rpc('resources/read', {'uri': 'freak-melee://examples/mario_full.fms'}, id=10),
        ]
        responses = [json.loads(line) for line in run('mcp', input='\n'.join(requests) + '\n').splitlines()]
        assert len(responses) == len(requests) - 1
        by_id = {r['id']: r for r in responses if r['id'] is not None}
        assert by_id[0]['error']['code'] == -32000
        assert by_id['init']['result']['protocolVersion'] == '2025-11-25'
        assert {t['name'] for t in by_id[2]['result']['tools']} == names
        assert by_id[3]['result']['structuredContent']['installation'] == str(tool.parent)
        assert by_id[4]['error']['code'] == -32602
        assert len(by_id[5]['result']['resources']) >= 85
        assert '# Fighter scripting' in by_id[6]['result']['contents'][0]['text']
        assert by_id[7]['error']['code'] == -32002
        assert by_id[8]['error']['code'] == -32601
        assert [r['error']['code'] for r in responses if r['id'] is None] == [-32700, -32700]
        assert by_id[9]['result'] == {}
        assert by_id[10]['result']['contents'][0]['mimeType'] == 'text/plain'
        assert 'fighter:' in by_id[10]['result']['contents'][0]['text']
        # The installed entry remains stable while state.json selects another
        # build. No updater, ISO picker or visible client may run in MCP mode.
        installation = folder / 'Installation with spaces'
        installation.mkdir()
        launcher = Path(sys.argv[3]).resolve() if len(sys.argv) > 3 else Path(__file__).resolve().parents[2] / 'tools/freak_launcher.ps1'
        shutil.copyfile(launcher, installation / 'freak_launcher.ps1')
        (installation / 'channel.txt').write_text('http://127.0.0.1:1/latest.json')
        powershell = Path(os.environ['SystemRoot']) / 'System32/WindowsPowerShell/v1.0/powershell.exe'
        for build in ('first', 'second'):
            version = installation / 'versions' / build
            version.mkdir(parents=True)
            for file in [tool, *tool.parent.glob('*.dll')]:
                # Real installations have independent files. Hardlinks couple
                # cleanup to another running client's mapped DLLs on Windows.
                shutil.copyfile(file, version / file.name)
            (installation / 'state.json').write_text(json.dumps({'current': build}))
            replies = asyncio.run(launcher_exchange([str(powershell), '-NoProfile', '-NonInteractive',
                '-ExecutionPolicy', 'Bypass', '-File', str(installation / 'freak_launcher.ps1'), '-AgentMcp'],
                env, requests[1], requests[4]))
            assert len(replies) == 2
            assert replies[1]['result']['structuredContent']['installation'] == str(version)
        print('PASS: structured discovery, real draft/source inspection, unchanged assets, '
              'pagination, MCP lifecycle, resources, errors, bounded framing and active-version launcher')


if __name__ == '__main__':
    main()
