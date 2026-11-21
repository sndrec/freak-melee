"""Exercise revisioned source batches, real Windows ownership and recovery."""
import ctypes
from ctypes import wintypes as w
from contextlib import contextmanager
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile


kernel = ctypes.WinDLL('kernel32', use_last_error=True)
kernel.CreateFileW.argtypes = [w.LPCWSTR, w.DWORD, w.DWORD, ctypes.c_void_p, w.DWORD, w.DWORD, w.HANDLE]
kernel.CreateFileW.restype = w.HANDLE
kernel.CloseHandle.argtypes = [w.HANDLE]
kernel.CreateMutexW.argtypes = [ctypes.c_void_p, w.BOOL, w.LPCWSTR]
kernel.CreateMutexW.restype = w.HANDLE
kernel.WaitForSingleObject.argtypes = [w.HANDLE, w.DWORD]
kernel.ReleaseMutex.argtypes = [w.HANDLE]


class FileInfo(ctypes.Structure):
    _fields_ = [('attributes', w.DWORD), ('created', w.FILETIME),
                ('accessed', w.FILETIME), ('written', w.FILETIME),
                ('volume', w.DWORD), ('size_high', w.DWORD), ('size_low', w.DWORD),
                ('links', w.DWORD), ('index_high', w.DWORD), ('index_low', w.DWORD)]


kernel.GetFileInformationByHandle.argtypes = [w.HANDLE, ctypes.POINTER(FileInfo)]


@contextmanager
def directory_lease(directory):
    handle = kernel.CreateFileW(str(directory), 0x80, 7, None, 3, 0x02000000, None)
    assert handle not in (None, ctypes.c_void_p(-1).value)
    info = FileInfo()
    try:
        assert kernel.GetFileInformationByHandle(handle, ctypes.byref(info))
    finally:
        kernel.CloseHandle(handle)
    name = f'Local\\FreakMeleeDraft-{info.volume:08x}-{info.index_high:08x}{info.index_low:08x}'
    mutex = kernel.CreateMutexW(None, False, name)
    assert mutex and kernel.WaitForSingleObject(mutex, 0) in (0, 0x80)
    try:
        yield
    finally:
        assert kernel.ReleaseMutex(mutex)
        kernel.CloseHandle(mutex)


@contextmanager
def undeletable_file(path):
    # Reads can proceed, but a destination replacement cannot. This causes a
    # genuine failure after the batch has replaced its first file.
    handle = kernel.CreateFileW(str(path), 0x80000000, 1, None, 3, 0x80, None)
    assert handle not in (None, ctypes.c_void_p(-1).value)
    try:
        yield
    finally:
        kernel.CloseHandle(handle)


def snapshot(directory):
    return {p.relative_to(directory).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in directory.rglob('*') if p.is_file()}


def main():
    tool, iso = [Path(arg).resolve() for arg in sys.argv[1:3]]
    with tempfile.TemporaryDirectory(prefix='agent-edit-', dir=tool.parent) as temporary:
        folder = Path(temporary)
        (folder / 'userdata').mkdir()
        windows = Path(os.environ['SystemRoot'])
        env = dict(os.environ, LOCALAPPDATA=str(folder / 'userdata'),
                   PATH=str(windows / 'System32') + ';' + str(windows))
        def run(*args, expected=0, input=None):
            result = subprocess.run([str(tool), *map(str, args)], env=env, cwd=folder,
                input=input, encoding='utf-8', capture_output=True, timeout=90,
                creationflags=subprocess.CREATE_NO_WINDOW)
            assert result.returncode == expected, (args[:3], result.stdout, result.stderr)
            return result.stdout
        def call(name, arguments, expected=0):
            response = json.loads(run('agent', 'call', name, '--stdin',
                input=json.dumps(arguments), expected=expected))
            assert response['ok'] == (expected == 0), response
            return response['result'] if response['ok'] else response['error']

        draft = folder / 'Mario'
        run('clone-stock', iso, 'Mario', draft, 'Editable Mario')
        args = {'project': str(draft)}
        listing = call('scripts.list', args)
        revision = listing['revision']
        originals = {f['file'].replace('\\', '/'): call('scripts.read', dict(args, file=f['file']))['text']
                     for f in listing['files']}
        names = list(originals)
        before = snapshot(draft)
        edits = [{'file': name, 'text': originals[name] + '\n# agent batch\n'} for name in names[:2]]
        mutation = dict(args, revision=revision, files=edits,
                        parameters='int cpu_capture_first_attempt = 13\n')
        assert call('scripts.update', dict(mutation, files=[dict(edits[0], typo=True)]), expected=1)['code'] == 'invalid_arguments'
        assert call('scripts.update', dict(mutation, files=[edits[0], {'file': '../outside.fms', 'text': 'no'}]), expected=1)['code'] == 'unknown_source'
        assert call('scripts.update', dict(mutation, files=[edits[0], edits[0]]), expected=1)['code'] == 'duplicate_source'
        assert call('scripts.update', dict(mutation, parameters='nonsense'), expected=1)['code'] in ('parameter_import', 'invalid_parameters')
        assert snapshot(draft) == before
        with directory_lease(draft):
            assert call('scripts.update', dict(mutation, project=str(draft).swapcase()), expected=1)['code'] == 'draft_busy'
            run('project-attribute', draft, 'walk_initial_velocity', '1', expected=1)
        changed = call('scripts.update', mutation)
        assert changed['status'] == 'saved_draft' and not changed['validated'] and not changed['installed']
        assert changed['previous_revision'] == revision and changed['revision'] != revision
        assert call('scripts.check', args)['accepted']
        for edit in edits:
            assert (draft / edit['file']).read_text() == edit['text']
        stable = snapshot(draft)
        assert call('scripts.update', mutation, expected=1)['code'] == 'revision_conflict'
        assert snapshot(draft) == stable
        bad = edits[0]['text'] + '\nfunc BadOne():\n    missing.one()\n\nfunc BadTwo():\n    missing.two()\n'
        broken = call('scripts.update', dict(args, revision=changed['revision'], files=[{'file': names[0], 'text': bad}]))
        diagnostics = call('scripts.check', args)
        assert not diagnostics['accepted'] and diagnostics['diagnostic_count'] >= 2
        fixed = call('scripts.update', dict(args, revision=broken['revision'], files=edits))
        assert call('scripts.check', args)['accepted']

        # Existing override errors can be repaired through the same operation.
        (draft / 'fighter_parameters.fmp').write_text('bool cpu_capture_first_attempt = true\n')
        broken_revision = call('project.inspect', args)['revision']
        fixed = call('scripts.update', dict(args, revision=broken_revision, files=[],
                                            parameters='int cpu_capture_first_attempt = 13\n'))
        assert call('scripts.check', args)['accepted']
        before = snapshot(draft)
        with undeletable_file(draft / names[1]):
            failed = call('scripts.update', dict(args, revision=fixed['revision'], files=[
                {'file': name, 'text': originals[name] + '\n# failed batch\n'} for name in names[:2]]), expected=1)
            assert failed['code'] == 'edit_failed', failed
            assert (draft / '.workshop-transaction/prepared').exists()
        # The next exclusive open completes recovery before opening assets.
        recovered = call('scripts.update', dict(args, revision=fixed['revision'], files=edits))
        assert not (draft / '.workshop-transaction').exists()
        assert snapshot(draft) == before
        assert recovered['revision'] == fixed['revision']

        # Model an abruptly terminated process: its durable prepared journal
        # survives, while one destination already contains the new bytes.
        journal = draft / '.workshop-transaction'
        journal.mkdir()
        (journal / 'owner').write_bytes(b'Freak Melee authoring transaction\n')
        old = (draft / names[0]).read_bytes()
        relative = names[0].encode()
        (journal / '0.old').write_bytes(old)
        (journal / 'manifest').write_bytes(b'FMWORKTX' + struct.pack('<III', 1, len(relative), 1) + relative)
        (journal / 'prepared').write_text('ready')
        (draft / names[0]).write_text('# interrupted partial write\n')
        # An incomplete backup set must leave the current draft available
        # exactly as it stood when recovery started.
        second = names[1].encode()
        complete_manifest = (journal / 'manifest').read_bytes()
        (journal / 'manifest').write_bytes(b'FMWORKTX' + struct.pack('<I', 2) +
            complete_manifest[12:] + struct.pack('<II', len(second), 1) + second)
        assert call('scripts.update', dict(args, revision=recovered['revision'], files=edits), expected=1)['code'] == 'draft_busy'
        assert (draft / names[0]).read_text() == '# interrupted partial write\n'
        (journal / 'manifest').write_bytes(complete_manifest)
        call('scripts.update', dict(args, revision=recovered['revision'], files=edits))
        assert snapshot(draft) == before and not journal.exists()
        # A corrupt journal remains available for inspection and blocks writes.
        journal.mkdir()
        (journal / 'owner').write_bytes(b'Freak Melee authoring transaction\n')
        (journal / 'manifest').write_bytes(b'corrupt')
        (journal / 'prepared').write_text('ready')
        assert call('scripts.update', dict(args, revision=recovered['revision'], files=edits), expected=1)['code'] == 'draft_busy'
        assert (journal / 'manifest').read_bytes() == b'corrupt'
        for path in journal.iterdir(): path.unlink()
        journal.rmdir()
        journal.mkdir()
        (journal / 'user-file.txt').write_text('preserve this unrelated file')
        assert call('scripts.update', dict(args, revision=recovered['revision'], files=edits), expected=1)['code'] == 'draft_busy'
        assert (journal / 'user-file.txt').read_text() == 'preserve this unrelated file'
        (journal / 'user-file.txt').unlink()
        journal.rmdir()

        # The stdio MCP transport uses the same nested schema, revision guard
        # and authoring operation as the CLI.
        rpc = lambda method, params, identity: dict(jsonrpc='2.0', method=method, params=params, id=identity)
        requests = [rpc('initialize', {'protocolVersion': '2025-11-25', 'capabilities': {},
                    'clientInfo': {'name': 'agent-edit-test', 'version': 'test'}}, 1),
            {'jsonrpc': '2.0', 'method': 'notifications/initialized'},
            rpc('tools/call', {'name': 'scripts.update', 'arguments': dict(args,
                revision=recovered['revision'], files=[{'file': names[0], 'text': edits[0]['text'] + '# MCP edit\n'}])}, 2),
            rpc('tools/call', {'name': 'scripts.check', 'arguments': args}, 3)]
        replies = {row['id']: row for row in map(json.loads,
            run('mcp', input='\n'.join(map(json.dumps, requests)) + '\n').splitlines())}
        assert replies[2]['result']['structuredContent']['committed']
        assert not replies[2]['result'].get('isError')
        assert replies[3]['result']['structuredContent']['accepted']

        pair = folder / 'Zelda'
        run('clone-stock', iso, 'Zelda', pair, 'Editable Zelda')
        run('clone-stock', iso, 'Sheik', pair / 'partner', 'Editable Sheik')
        (pair / 'family.txt').write_text('mode=alternate\nprimary_key=zelda\npartner_key=sheik\npartner=partner\n')
        pair_info = call('project.inspect', {'project': str(pair)})
        assert len(pair_info['members']) == 2
        with directory_lease(Path(pair_info['members'][1]['path'])):
            denied = call('scripts.update', {'project': str(pair), 'revision': pair_info['revision'],
                'files': [], 'parameters': ''}, expected=1)
            assert denied['code'] == 'draft_busy'
        print('PASS: CLI/MCP source/parameter batches, stale/invalid edits, Windows leases, family ownership, rollback and interrupted journal recovery')


if __name__ == '__main__':
    main()
