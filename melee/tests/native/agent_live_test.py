"""Exercise unsaved Workshop edits in one isolated hidden, muted native client."""
import argparse
import ctypes
from ctypes import wintypes as w
import hashlib
import json
import os
import re
from pathlib import Path
import subprocess
import time


def close_window(pid):
    user = ctypes.WinDLL('user32', use_last_error=True)
    callback = ctypes.WINFUNCTYPE(w.BOOL, w.HWND, w.LPARAM)
    user.GetWindowThreadProcessId.argtypes = [w.HWND, ctypes.POINTER(w.DWORD)]
    user.PostMessageW.argtypes = [w.HWND, w.UINT, w.WPARAM, w.LPARAM]
    @callback
    def visit(window, _):
        owner = w.DWORD()
        user.GetWindowThreadProcessId(window, ctypes.byref(owner))
        if owner.value == pid: user.PostMessageW(window, 0x10, 0, 0)
        return True
    user.EnumWindows(visit, 0)


def snapshot(root):
    return {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in root.rglob('*') if p.is_file()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for field in ('exe', 'tool', 'iso', 'output'):
        parser.add_argument('--' + field, type=Path, required=True)
    parser.add_argument('--old-tool', type=Path)
    parser.add_argument('--control', type=Path, help='Optional developer input driver for the manual-text/visual check')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    appdata = output / 'userdata'
    appdata.mkdir()
    windows = Path(os.environ['SystemRoot'])
    env = dict(os.environ, LOCALAPPDATA=str(appdata),
               PATH=str(windows / 'System32') + ';' + str(windows))
    for service in ('CHAT', 'SERVICE', 'COMMUNITY'):
        env['FREAK_MELEE_' + service + '_ENDPOINT'] = 'http://127.0.0.1:1'
    control_id = 'agent-live-' + str(os.getpid())
    env['FREAK_MELEE_CONTROL_ID'] = control_id
    env['FREAK_MELEE_MENU_MOTION'] = '0'
    tool = args.tool.resolve()
    exe = args.exe.resolve()
    def run(*parts, input=None, executable=tool):
        result = subprocess.run([str(executable), *map(str, parts)], input=input, env=env, cwd=output,
            encoding='utf-8', capture_output=True, timeout=90, creationflags=subprocess.CREATE_NO_WINDOW)
        return result
    def call(name, arguments=None, expected=True, executable=tool):
        result = run('agent', 'call', name, '--stdin', input=json.dumps(arguments or {}), executable=executable)
        assert result.stdout, (name, result.returncode, result.stderr)
        reply = json.loads(result.stdout)
        assert reply['ok'] == expected, (name, reply)
        return reply['result'] if expected else reply['error']
    project = appdata / 'Freak Melee/projects/Live Mario'
    project.parent.mkdir(parents=True, exist_ok=True)
    cloned = run('clone-stock', args.iso.resolve(), 'Mario', project, 'Live Mario')
    assert cloned.returncode == 0, cloned.stderr
    saved = {'project': str(project)}
    files = call('scripts.list', saved)
    first = files['files'][0]['file']
    first_text = call('scripts.read', dict(saved, file=first))['text']
    call('scripts.update', dict(saved, revision=files['revision'], files=[{'file': first, 'text': first_text}]))
    combined = ''
    for file in call('scripts.list', saved)['files']:
        text = call('scripts.read', dict(saved, file=file['file']))['text']
        if combined and not combined.endswith('\n'): combined += '\n'
        combined += text
    source = output / 'combined.fms'
    source.write_text(combined, encoding='utf-8')
    built = run('script-build', project, source)
    assert built.returncode == 0, built.stderr
    baseline = snapshot(project)
    content = appdata / 'Freak Melee/content'
    content.mkdir(parents=True, exist_ok=True)
    (content / 'startup.txt').write_text('12 0 1\n\n')
    (content / 'hub-profile.txt').write_text('Agent review\n\n')
    process = None
    manual_input_checked = False
    def control(*parts):
        result = subprocess.run([str(args.control.resolve()), '--id', control_id, *map(str, parts)],
            env=env, cwd=output, capture_output=True, text=True, timeout=10, creationflags=subprocess.CREATE_NO_WINDOW)
        assert result.returncode == 0, result.stderr
    def position(element):
        log = output / 'stderr.log'
        for _ in range(30):
            start = log.stat().st_size
            control('ui-inspect', element)
            time.sleep(.15)
            with log.open('r', encoding='utf-8', errors='replace') as stream:
                stream.seek(start)
                text = stream.read(2 * 1024 * 1024)
            found = re.search(r'\[ui-inspect\] ' + re.escape(element) +
                r':?[^\n]*origin=([\d.-]+),([\d.-]+) size=([\d.-]+),([\d.-]+)', text)
            if found:
                x, y, width, height = map(float, found.groups())
                if width > 0 and height > 0 and 'visible=0' not in found[0]:
                    return round(x + width / 2), round(y + height / 2)
        raise AssertionError(f'UI control unavailable: {element}')
    def ui_text(element):
        log = output / 'stderr.log'
        for _ in range(30):
            start = log.stat().st_size
            control('ui-inspect', element)
            time.sleep(.15)
            with log.open('r', encoding='utf-8', errors='replace') as stream:
                stream.seek(start)
                text = stream.read(2 * 1024 * 1024)
            found = re.search(r'\[ui-inspect-value\] ' + re.escape(element) + r':[^\n]* text=([^\n]*)', text)
            if found: return found[1].strip()
        raise AssertionError(f'No UI text for {element}')
    try:
        with (output / 'stdout.log').open('w') as stdout, (output / 'stderr.log').open('w') as stderr:
            process = subprocess.Popen([str(exe), '--iso', str(args.iso.resolve()), '--hidden', '--mute'],
                env=env, cwd=output, stdout=stdout, stderr=stderr, creationflags=subprocess.CREATE_NO_WINDOW)
        (output / 'pid.txt').write_text(str(process.pid))
        deadline = time.monotonic() + 120
        instance = None
        while time.monotonic() < deadline:
            assert process.poll() is None, f'Game exited: {process.returncode}; inspect {output}'
            matches = [row for row in call('instances.list')['instances'] if row['process_id'] == process.pid]
            if matches and matches[0]['available']:
                assert matches[0]['build_matches'], matches[0]
                assert Path(matches[0]['executable']) == exe, matches[0]
                instance = matches[0]['instance']
                break
            time.sleep(.25)
        assert instance, f'No endpoint for the live process; inspect {output}'
        target = {'instance': instance}
        state = call('session.inspect', target)
        assert not state['workshop_open'] and state['project'] is None, state
        if args.old_tool:
            old = call('session.inspect', target, expected=False, executable=args.old_tool.resolve())
            assert old['code'] == 'build_mismatch', old
        # Opening is issued once. If its response is lost, inspect this exact
        # process instead of issuing the mutation again.
        opened = run('agent', 'call', 'session.open', '--stdin',
            input=json.dumps(dict(target, revision=state['revision'], project=str(project))))
        assert opened.stdout, opened.stderr
        open_reply = json.loads(opened.stdout)
        (output / 'session-open.json').write_text(json.dumps(open_reply, indent=2))
        if not open_reply['ok']:
            assert open_reply['error']['code'] == 'completion_unknown', open_reply
        deadline = time.monotonic() + 180
        while time.monotonic() < deadline:
            assert process.poll() is None, f'Game exited: {process.returncode}; inspect {output}'
            result = run('agent', 'call', 'session.inspect', '--stdin', input=json.dumps(target))
            if result.stdout:
                response = json.loads(result.stdout)
                if response['ok']:
                    state = response['result']
                    if state.get('project') == str(project) and state['editing_allowed']: break
                elif response['error']['code'] not in ('completion_unknown', 'instance_unavailable', 'request_not_sent'):
                    raise AssertionError(('session.inspect', response))
            time.sleep(.25)
        else: raise AssertionError(f'Workshop preview did not become editable; inspect {output}')
        assert state['workshop_open'] and state['member'] == 'main'
        assert state['preview_scene'] == 'practice', state
        def wait_preview(package, testing=False):
            deadline = time.monotonic() + 180
            while time.monotonic() < deadline:
                assert process.poll() is None, f'Game exited while loading preview: {process.returncode}'
                result = run('agent', 'call', 'session.inspect', '--stdin', input=json.dumps(target))
                if result.stdout:
                    response = json.loads(result.stdout)
                    if response['ok']:
                        latest = response['result']
                        if latest['preview_ready'] and latest['loaded_preview_package_id'] == package and latest['testing'] == testing:
                            return latest
                    elif response['error']['code'] not in ('completion_unknown', 'instance_unavailable', 'request_not_sent'):
                        raise AssertionError(response)
                time.sleep(.15)
            raise AssertionError(f'Preview did not load {package}; inspect {output}')
        state = wait_preview(state['requested_preview_package_id'])
        initial_package = state['loaded_preview_package_id']
        state = call('session.workspace.select', dict(target, revision=state['revision'], workspace='scripting'))
        assert state['workspace'] == 'scripting'
        listing = call('session.scripts.list', target)
        original = call('session.scripts.read', dict(target, file=first))
        assert listing['revision'] == original['revision']
        # A second inspection without an edit must describe the same revision.
        assert call('session.inspect', target)['revision'] == original['revision']
        original_text = original['text']
        original_parameters = listing['parameters']
        changed_text = original_text + '\n# live agent edit\n'
        changed = call('session.scripts.update', dict(target, revision=original['revision'],
            summary='Add live authoring note', files=[{'file': first, 'text': changed_text}]))
        assert changed['committed'] and not changed['saved'] and not changed['installed']
        assert changed['revision'] != original['revision'] and changed['dirty']
        assert changed['can_undo']
        assert call('session.scripts.read', dict(target, file=first))['text'] == changed_text
        checked = call('session.scripts.check', dict(target, revision=changed['revision']))
        assert checked['accepted'] and checked['draft_view'] == 'live_snapshot', checked
        assert checked['revision'] == changed['revision'] and checked['dirty']
        assert call('session.inspect', target)['revision'] == changed['revision']
        broken_text = changed_text + '\nfunc AgentBadOne():\n    missing.one()\n\nfunc AgentBadTwo():\n    missing.two()\n'
        broken = call('session.scripts.update', dict(target, revision=changed['revision'], summary='Check unfinished source',
            files=[{'file': first, 'text': broken_text}]))
        problems = call('session.scripts.check', dict(target, revision=broken['revision']))
        assert not problems['accepted'] and problems['diagnostic_count'] >= 2, problems
        assert all(d['file'].replace('\\', '/') == first.replace('\\', '/') and d['line'] > 0 and
                   d['source_view'] == 'session.scripts.read' for d in problems['diagnostics']), problems
        assert problems['revision'] == broken['revision']
        if args.control:
            diagnostic = problems['diagnostics'][0]
            control('screenshot')
            control('ui-click', *position('workshop-script-problem-0-' + str(diagnostic['line'])))
            selected = call('session.inspect', target)
            assert selected['selected_source'] == first, selected
            broken['revision'] = selected['revision']
        changed = call('session.undo', dict(target, revision=broken['revision']))
        assert call('session.scripts.read', dict(target, file=first))['text'] == changed_text
        if args.control:
            # Checking through the real button must replace the previous errors
            # without the old event handler's forced disk save.
            control('ui-click', *position('workshop-script-validate'))
            assert ui_text('workshop-script-problems') == 'No problems found.'
        assert snapshot(project) == baseline, 'Live edits wrote disk files'
        denied = call('scripts.update', dict(saved, revision=call('project.inspect', saved)['revision'],
            files=[{'file': first, 'text': 'do not overwrite the live draft'}]), expected=False)
        assert denied['code'] == 'draft_busy'
        stale = call('session.scripts.update', dict(target, revision=original['revision'],
            summary='Stale edit', files=[{'file': first, 'text': 'do not overwrite'}]), expected=False)
        assert stale['code'] == 'revision_conflict' and stale['revision'] == changed['revision']
        rejected = call('session.scripts.update', dict(target, revision=changed['revision'], summary='Invalid batch',
            files=[{'file': first, 'text': 'do not commit this either'}, {'file': '../missing.fms', 'text': 'no'}]), expected=False)
        assert rejected['code'] == 'unknown_source'
        assert call('session.scripts.read', dict(target, file=first))['text'] == changed_text
        undone = call('session.undo', dict(target, revision=changed['revision']))
        assert call('session.scripts.read', dict(target, file=first))['text'] == original_text
        assert call('session.scripts.list', target)['parameters'] == original_parameters
        redone = call('session.redo', dict(target, revision=undone['revision']))
        assert call('session.scripts.read', dict(target, file=first))['text'] == changed_text
        # Source and parameters share the same history step.
        parameter = re.search(r'(?m)^int cpu_capture_first_attempt = (-?\d+)$', original_parameters)
        assert parameter, original_parameters[:1000]
        replacement_parameters = original_parameters[:parameter.start(1)] + str(int(parameter[1]) + 1) + original_parameters[parameter.end(1):]
        parameter_edit = call('session.scripts.update', dict(target, revision=redone['revision'],
            summary='Edit live parameter', files=[{'file': first, 'text': changed_text + '# another change\n'}],
            parameters=replacement_parameters))
        assert call('session.scripts.list', target)['parameters'] == replacement_parameters
        reverted = call('session.undo', dict(target, revision=parameter_edit['revision']))
        assert call('session.scripts.read', dict(target, file=first))['text'] == changed_text
        assert call('session.scripts.list', target)['parameters'] == original_parameters
        assert snapshot(project) == baseline
        if args.control:
            reverted = call('session.scripts.select', dict(target, revision=reverted['revision'], file=first))
            # The old input driver is used only to simulate a human typing.
            # All discovery, authoring, history and verification use the shipped API.
            control('ui-click', *position('workshop-script-source'))
            control('ui-key', 'end', 'ctrl')
            control('ui-text', '\n# manual input check\n')
            for _ in range(30):
                manual = call('session.scripts.read', dict(target, file=first))
                if '# manual input check' in manual['text']: break
                time.sleep(.1)
            else: raise AssertionError('Manual textarea input was not visible to the live source reader')
            stale = call('session.undo', dict(target, revision=reverted['revision']), expected=False)
            assert stale['code'] == 'revision_conflict'
            preserved = call('session.undo', dict(target, revision=manual['revision']), expected=False)
            assert preserved['code'] == 'history_unavailable'
            assert call('session.scripts.read', dict(target, file=first))['text'] == manual['text']
            reverted = call('session.scripts.update', dict(target, revision=manual['revision'],
                summary='Remove test typing', files=[{'file': first, 'text': changed_text}]))
            manual_input_checked = True
            control('ui-click', *position('workshop-fighter-toggle'))
            control('ui-click', *position('workshop-agent-show'))
            position('workshop-agent-close')
            control('screenshot')
            time.sleep(.5)
            control('ui-click', *position('workshop-agent-disable'))
            for _ in range(30):
                if all(row['instance'] != instance for row in call('instances.list')['instances']): break
                time.sleep(.1)
            else: raise AssertionError('The in-game disconnect control did not close the endpoint')
            control('ui-click', *position('workshop-agent-enable'))
            for _ in range(30):
                endpoints = [row for row in call('instances.list')['instances'] if row['process_id'] == process.pid and row['available']]
                if endpoints: break
                time.sleep(.1)
            else: raise AssertionError('The in-game enable control did not open an endpoint')
            assert endpoints[0]['instance'] != instance
            instance = endpoints[0]['instance']
            target = {'instance': instance}
            connected = call('session.inspect', target)
            assert connected['project'] == str(project) and connected['revision'] == reverted['revision']
            control('ui-click', *position('workshop-agent-close'))
        # Save owns the whole family transaction. External edits are detected
        # against the revision captured when this draft opened, not accepted
        # silently as a new baseline immediately before writing.
        state = call('session.inspect', target)
        external = project / 'external-note.txt'
        external.write_text('Preserve this external change.\n', encoding='utf-8')
        external_snapshot = snapshot(project)
        refused = call('session.save', dict(target, revision=state['revision']), expected=False)
        assert refused['code'] == 'save_failed' and 'outside' in refused['message'], refused
        assert snapshot(project) == external_snapshot
        assert call('session.scripts.read', dict(target, file=first))['text'] == changed_text
        external.unlink()
        assert snapshot(project) == baseline
        final_text = changed_text + '# saved live transaction\n'
        edited = call('session.scripts.update', dict(target, revision=state['revision'],
            summary='Save a complete live draft', files=[{'file': first, 'text': final_text}],
            parameters=replacement_parameters))
        saved_live = call('session.save', dict(target, revision=edited['revision']))
        assert saved_live['saved'] and not saved_live['installed'] and not saved_live['dirty'], saved_live
        assert (project / first).read_text(encoding='utf-8') == final_text
        assert (project / 'fighter_parameters.fmp').read_text(encoding='utf-8') == replacement_parameters
        assert snapshot(project)['behavior.fms'] == baseline['behavior.fms'], 'Save changed accepted behavior'
        assert not (project / '.workshop-transaction').exists()
        assert call('project.inspect', saved)['revision'] == saved_live['saved_disk_revision']
        undone = call('session.undo', dict(target, revision=saved_live['revision']))
        assert undone['dirty'], undone
        reverted = call('session.redo', dict(target, revision=undone['revision']))
        assert not reverted['dirty'], reverted
        assert call('session.scripts.read', dict(target, file=first))['text'] == final_text
        # Action editing reads the unsaved archive, owns shared commands, and
        # commits one property group into the same editor history as the UI.
        actions = call('session.actions.list', dict(target, query='AttackAirLw'))
        dair = next(row['id'] for row in actions['actions'] if row['name'] == 'AttackAirLw')
        events = call('session.actions.read', dict(target, view='events', action_id=dair))
        hit = next(row for row in events['events'] if row['kind'] == 'hitbox')
        previous = call('session.actions.read', dict(target, view='event', action_id=dair, event_id=hit['event_id']))
        other_hit = next(row for row in events['events'] if row['kind'] == 'hitbox' and row['event_id'] != hit['event_id'])
        other_previous = call('session.actions.read', dict(target, view='event', action_id=dair, event_id=other_hit['event_id']))
        action_disk = snapshot(project)
        action_edit = call('session.actions.edit', dict(target, revision=previous['revision'], action_id=dair,
            operation='update', summary='Edit aerial damage and element together', updates=[
                {'event_id': hit['event_id'], 'properties': {'damage': previous['properties']['damage'] + 1, 'element': 2}},
                {'event_id': other_hit['event_id'], 'properties': {'damage': other_previous['properties']['damage'] + 2}}]))
        assert action_edit['workspace'] == 'action' and action_edit['can_undo'] and action_edit['dirty']
        assert not action_edit['saved'] and not action_edit['installed']
        updated = call('session.actions.read', dict(target, view='event', action_id=dair, event_id=action_edit['event_id']))
        assert updated['properties']['damage'] == previous['properties']['damage'] + 1 and updated['properties']['element'] == 2
        assert snapshot(project) == action_disk
        reverted_action = call('session.undo', dict(target, revision=updated['revision']))
        restored = call('session.actions.read', dict(target, view='event', action_id=dair, event_id=hit['event_id']))
        assert restored['properties'] == previous['properties']
        assert call('session.actions.read', dict(target, view='event', action_id=dair, event_id=other_hit['event_id']))['properties'] == other_previous['properties']
        reverted = call('session.redo', dict(target, revision=reverted_action['revision']))
        assert call('session.actions.read', dict(target, view='event', action_id=dair,
            event_id=action_edit['event_id']))['properties']['element'] == 2
        if args.control:
            before_events = call('session.actions.read', dict(target, view='events', action_id=dair))
            control('ui-click', *position('workshop-add-event'))
            position('workshop-add-kind-2')
            control('screenshot')
            control('ui-click', *position('workshop-add-kind-2'))
            for _ in range(30):
                new_events = call('session.actions.read', dict(target, view='events', action_id=dair))
                if new_events['total'] == before_events['total'] + 1: break
                time.sleep(.1)
            else: raise AssertionError('UI Add hitbox did not create an event')
            created = next(row for row in new_events['events'] if row['kind'] == 'hitbox' and row['first_frame'] == 0)
            properties = call('session.actions.read', dict(target, view='event', action_id=dair, event_id=created['event_id']))
            assert properties['properties']['timed'] and properties['properties']['duration'] == 1
            control('screenshot')
            reverted = call('session.undo', dict(target, revision=properties['revision']))
            assert call('session.actions.read', dict(target, view='events', action_id=dair))['total'] == before_events['total']
        labeled = call('session.actions.edit', dict(target, revision=reverted['revision'], action_id=dair,
            operation='metadata', label='Orbital kicks', hidden=True, summary='Name and hide an action'))
        assert labeled['dirty'] and labeled['label'] == 'Orbital kicks' and labeled['hidden']
        assert snapshot(project) == action_disk
        if args.control: assert ui_text('workshop-context-action') == 'Orbital kicks'
        undone_label = call('session.undo', dict(target, revision=labeled['revision']))
        assert not call('session.actions.read', dict(target, view='metadata', action_id=dair))['hidden']
        reverted = call('session.redo', dict(target, revision=undone_label['revision']))
        assert call('session.actions.read', dict(target, view='metadata', action_id=dair))['label'] == 'Orbital kicks'
        built = call('session.build', dict(target, revision=reverted['revision']))
        assert built['accepted'] and built['saved'] and not built['installed'] and not built['dirty'], built
        assert built['loaded_preview_package_id'] == initial_package and not built['preview_matches_fighter'], built
        assert all(row['accepted'] for row in built['member_reports'])
        assert call('actions.read', dict(saved, action_id=dair, view='metadata'))['label'] == 'Orbital kicks'
        accepted_disk = snapshot(project)
        preview_text = final_text + '# unsaved preview snapshot\n'
        draft = call('session.scripts.update', dict(target, revision=built['revision'], summary='Preview unsaved source',
            files=[{'file': first, 'text': preview_text}]))
        refreshed = call('session.preview.refresh', dict(target, revision=draft['revision']))
        assert not refreshed['saved'] and not refreshed['installed']
        frozen = wait_preview(refreshed['package_id'])
        assert frozen['preview_matches_fighter'] and frozen['fighter_revision'] == refreshed['fighter_revision']
        assert snapshot(project) == accepted_disk
        practice = call('session.practice.start', dict(target, revision=frozen['revision']))
        playing = wait_preview(practice['package_id'], testing=True)
        denied = call('session.scripts.update', dict(target, revision=playing['revision'], summary='Refused during practice', files=[]), expected=False)
        assert denied['code'] == 'editor_busy'
        stopped = call('session.practice.stop', dict(target, revision=playing['revision']))
        frozen = wait_preview(playing['loaded_preview_package_id'])
        assert snapshot(project) == accepted_disk
        installed = call('session.install', dict(target, revision=frozen['revision']))
        assert installed['installed'] and installed['installed_package_id'] == installed['package_id'], installed
        assert installed['preview_ready'] and installed['loaded_preview_package_id'] == frozen['loaded_preview_package_id']
        exported = output / 'Live Mario.fmc'
        package = call('packages.export', {'package_id': installed['package_id'], 'output': str(exported)})
        assert hashlib.sha256(exported.read_bytes()).hexdigest() == installed['package_id']
        assert package['bytes'] == installed['package_bytes']
        disconnected = call('session.disconnect', target)
        assert disconnected['status'] == 'disconnected' and not disconnected['agent_access_enabled']
        assert process.poll() is None, 'Disconnect closed the game'
        time.sleep(.1)
        assert all(row['instance'] != instance for row in call('instances.list')['instances'])
        (output / 'result.json').write_text(json.dumps({'status': 'PASS', 'instance': instance,
            'build_identity': state['build_identity'], 'project': str(project), 'revision': installed['revision']}, indent=2))
        report = json.loads((output / 'result.json').read_text())
        report['manual_input_checked'] = manual_input_checked
        (output / 'result.json').write_text(json.dumps(report, indent=2))
        print('PASS: live source/action authoring, shared action undo, atomic acceptance, preview identity, practice, installation and exact export')
    finally:
        if process and process.poll() is None:
            close_window(process.pid)
            process.wait(timeout=30)


if __name__ == '__main__':
    main()
