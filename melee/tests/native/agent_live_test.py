"""Exercise unsaved Workshop edits in one isolated hidden, muted native client."""
import argparse
import base64
import ctypes
from ctypes import wintypes as w
import hashlib
import json
import os
import re
import struct
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
    mario = call('stock.list', {'query': 'Mario'})['fighters']
    mario = next(row for row in mario if row['name'] == 'Mario')
    cloned = call('project.clone_stock', {'iso': str(args.iso.resolve()), 'fighter_id': mario['fighter_id'],
        'output': str(project), 'title': 'Live Mario'})
    assert Path(cloned['project']) == project and len(cloned['members']) == 1
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
    def ui_text(element, value=False):
        log = output / 'stderr.log'
        for _ in range(30):
            start = log.stat().st_size
            control('ui-inspect', element)
            time.sleep(.15)
            with log.open('r', encoding='utf-8', errors='replace') as stream:
                stream.seek(start)
                text = stream.read(2 * 1024 * 1024)
            suffix = r':[^\n]* value=(.*?) text=' if value else r':[^\n]* text=([^\n]*)'
            found = re.search(r'\[ui-inspect-value\] ' + re.escape(element) + suffix, text)
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
        assert call('project.rename', dict(saved, revision=call('project.inspect', saved)['revision'],
            title='Do not rename an open draft'), expected=False)['code'] == 'draft_busy'
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
        # Common tuning shares native limits and the editor's undo history.
        tuning = call('session.attributes.list', dict(target, limit=200))
        values = {row['name']: row['value'] for row in tuning['attributes']}
        attribute_disk = snapshot(project)
        tuned = call('session.attributes.edit', dict(target, revision=tuning['revision'],
            summary='Tune gravity and jumping together', changes=[
                {'name': 'gravity', 'value': .125}, {'name': 'max_jumps', 'value': 3}]))
        assert tuned['changed'] and not tuned['saved'] and tuned['can_undo']
        assert snapshot(project) == attribute_disk
        invalid = call('session.attributes.edit', dict(target, revision=tuned['revision'],
            summary='Reject invalid tuning', changes=[{'name': 'weight', 'value': 100},
                {'name': 'max_jumps', 'value': 0}]), expected=False)
        assert invalid['code'] == 'attribute_edit_rejected'
        unchanged = call('session.attributes.list', dict(target, limit=200))
        assert unchanged['revision'] == tuned['revision']
        assert next(row['value'] for row in unchanged['attributes'] if row['name'] == 'weight') == values['weight']
        reverted = call('session.undo', dict(target, revision=tuned['revision']))
        restored = call('session.attributes.list', dict(target, limit=200))
        assert {row['name']: row['value'] for row in restored['attributes']} == values
        if args.control:
            control('ui-click', *position('workshop-package-toggle'))
            control('ui-click', *position('workshop-attribute-0'))
            control('ui-key', 'a', 'ctrl')
            control('ui-text', '0.375')
            assert ui_text('workshop-attribute-0', value=True) == '0.375'
            control('ui-key', 'enter')
            for _ in range(30):
                manual = call('session.attributes.list', dict(target, limit=200))
                if manual['attributes'][0]['value'] == .375: break
                time.sleep(.1)
            else: raise AssertionError((manual['attributes'][0], ui_text('workshop-editor-status'), ui_text('workshop-attribute-0', value=True)))
            reverted = call('session.undo', dict(target, revision=manual['revision']))
            assert abs(float(ui_text('workshop-attribute-0', value=True)) - values['walk_accel_mul']) < 1e-7
            control('ui-click', *position('workshop-attribute-0'))
            control('ui-key', 'a', 'ctrl')
            control('ui-text', '0.625')
            control('ui-key', 'escape')
            assert abs(float(ui_text('workshop-attribute-0', value=True)) - values['walk_accel_mul']) < 1e-7
            control('ui-click', *position('workshop-package-toggle'))
        assert snapshot(project) == attribute_disk
        model_mode = call('session.workspace.select', dict(target, revision=reverted['revision'], workspace='model'))
        model_tuned = call('session.attributes.edit', dict(target, revision=model_mode['revision'],
            summary='Tune weight from the Model workspace', changes=[{'name': 'weight', 'value': 150}]))
        assert model_tuned['workspace'] == 'model' and model_tuned['can_undo']
        model_undo = call('session.undo', dict(target, revision=model_tuned['revision']))
        restored = call('session.attributes.list', dict(target, limit=200))
        assert next(row['value'] for row in restored['attributes'] if row['name'] == 'weight') == values['weight']
        model_redo = call('session.redo', dict(target, revision=restored['revision']))
        assert next(row['value'] for row in call('session.attributes.list', dict(target, limit=200))['attributes'] if row['name'] == 'weight') == 150
        reverted = call('session.undo', dict(target, revision=model_redo['revision']))
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
        # Sampled draft data and the actor that is actually rendered must agree.
        animation_clip = {'bank': 'action', 'index': dair}
        rest = call('session.animations.edit', dict(target, revision=reverted['revision'], operation='create',
            target_action=dair, name='AgentDownAir', frames=24, summary='Create an independent preview clip'))
        bones = call('session.animations.read', dict(target, view='bones', clip=animation_clip, limit=200))['bones']
        hip = next(bone['id'] for bone in bones if bone.get('role') == 'HipN')
        keyed = call('session.animations.edit', dict(target, revision=rest['revision'], operation='insert', clip=animation_clip,
            summary='Key a visibly tilted pose', insertions=[{'bone': hip, 'channel': 'rotation_z', 'frame': 10, 'value': .5}]))
        selected = call('session.animations.select', dict(target, revision=keyed['revision'], clip=animation_clip, frame=10))
        def actor_pose(expected):
            for _ in range(50):
                result = run('agent', 'call', 'session.animations.preview', '--stdin', input=json.dumps(dict(target, bones=[hip])))
                reply = json.loads(result.stdout)
                if reply['ok']:
                    pose = reply['result']
                    if pose['clip'] == animation_clip and pose['frame'] == 10 and abs(pose['bones'][0]['rotation'][2] - expected) < .0001:
                        return pose
                else: assert reply['error']['code'] == 'preview_pending', reply
                time.sleep(.1)
            raise AssertionError('Edited animation did not reach the rendered preview actor')
        visible = actor_pose(.5)
        assert visible['workspace'] == 'animation' and snapshot(project) == action_disk
        configured = call('session.preview.configure', dict(target, revision=visible['revision'],
            camera=dict(target=[0, 8, 0], yaw=.55, pitch=.15, distance=36), bones=False))
        assert abs(configured['preview_view']['camera']['yaw'] - .55) < .00001
        assert not configured['preview_view']['bones']
        assert call('session.preview.configure', dict(target, revision=visible['revision'],
            bones=True), expected=False)['code'] == 'revision_conflict'
        def capture_image(scope, name, state):
            path = output / name
            queued = call('session.preview.capture', dict(target, revision=state['revision'],
                scope=scope, output=str(path)))
            assert queued['status'] == 'pending'
            for _ in range(80):
                receipt = call('session.preview.capture_status', dict(target, capture_id=queued['capture_id']))
                if receipt['status'] != 'pending': break
                time.sleep(.1)
            assert receipt['status'] == 'ready', receipt
            assert receipt['view'] == queued['view']
            assert receipt['view']['revision'] == state['revision']
            assert receipt['view']['preview_view']['camera'] == state['preview_view']['camera']
            pose = receipt['view']['preview_view']['pose']
            assert pose['frame'] == 10 and pose['index'] == dair
            png = path.read_bytes()
            assert png.startswith(b'\x89PNG\r\n\x1a\n')
            assert struct.unpack('>II', png[16:24]) == (receipt['width'], receipt['height'])
            assert hashlib.sha256(png).hexdigest() == receipt['png_sha256']
            path.with_suffix('.json').write_text(json.dumps(receipt, indent=2), encoding='utf-8')
            return receipt, png
        window, _ = capture_image('window', 'workshop-window.png', configured)
        view, png = capture_image('viewport', 'workshop-view.png', configured)
        assert window['width'] > view['width'] and window['height'] > view['height']
        assert view['displayed_frame'] > window['displayed_frame']
        assert abs(view['width'] * 3 - view['height'] * 4) <= 7
        assert call('session.preview.capture', dict(target, revision=configured['revision'],
            scope='viewport', output=view['output']), expected=False)['code'] == 'output_exists'
        def mcp_capture():
            init = dict(jsonrpc='2.0', id=1, method='initialize', params={'protocolVersion':'2025-11-25'})
            status = dict(jsonrpc='2.0', id=2, method='tools/call', params={
                'name':'session.preview.capture_status', 'arguments':dict(target, capture_id=view['capture_id'])})
            response = run('mcp', input=json.dumps(init) + '\n' + json.dumps(status) + '\n')
            assert response.returncode == 0, response.stderr
            return json.loads(response.stdout.splitlines()[1])['result']
        result = mcp_capture()
        image = next(block for block in result['content'] if block['type'] == 'image')
        assert image['mimeType'] == 'image/png' and base64.b64decode(image['data'], validate=True) == png
        assert {k:v for k,v in result['structuredContent'].items() if k != 'request_id'} == {
            k:v for k,v in view.items() if k != 'request_id'}
        Path(view['output']).write_bytes(bytes(len(png)))
        tampered = mcp_capture()
        assert tampered['structuredContent']['image_error'] and all(block['type'] != 'image' for block in tampered['content'])
        Path(view['output']).write_bytes(png)
        reset = call('session.preview.configure', dict(target, revision=configured['revision'],
            reset_camera='default'))
        camera2, _ = capture_image('viewport', 'workshop-view-reset.png', reset)
        assert camera2['png_sha256'] != view['png_sha256']
        playing = call('session.preview.configure', dict(target, revision=reset['revision'], playing=True))
        assert playing['preview_view']['playing']
        deadline = time.monotonic() + 3
        while time.monotonic() < deadline:
            advanced = call('session.inspect', target)
            if advanced['preview_view']['pose']['frame'] > 10:
                break
        else:
            raise AssertionError('Preview playback did not advance the actor')
        paused = call('session.preview.configure', dict(target, revision=advanced['revision'], playing=False))
        assert not paused['preview_view']['playing']
        call('session.animations.select', dict(target, revision=paused['revision'], clip=animation_clip, frame=10))
        visible = actor_pose(.5)
        assert snapshot(project) == action_disk
        if args.control: control('screenshot')
        undo_animation = call('session.undo', dict(target, revision=visible['revision']))
        after_undo = actor_pose(bones[hip]['rotation'][2])
        redone_animation = call('session.redo', dict(target, revision=after_undo['revision']))
        reverted = actor_pose(.5)
        built = call('session.build', dict(target, revision=reverted['revision']))
        assert built['accepted'] and built['saved'] and not built['installed'] and not built['dirty'], built
        # Animation history reloads the actor's package. Build itself preserves
        # that current actor rather than implicitly starting another preview.
        assert built['loaded_preview_package_id'] == reverted['loaded_preview_package_id'], built
        assert all(row['accepted'] for row in built['member_reports'])
        assert call('actions.read', dict(saved, action_id=dair, view='metadata'))['label'] == 'Orbital kicks'
        saved_pose = call('animations.read', dict(saved, view='pose', clip=animation_clip, frame=10, limit=200))
        assert saved_pose['pose'][hip]['rotation'][2] == .5
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
        presentation_before = call('presentation.read', saved)
        busy_edit = call('presentation.edit', dict(saved, revision=presentation_before['revision'],
            tile_name='LIVE'), expected=False)
        assert busy_edit['code'] == 'draft_busy', busy_edit
        opened = call('session.presentation.open', dict(target, revision=installed['revision']))
        assert opened['portrait_open'], opened
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline:
            portrait = call('session.presentation.inspect', target)
            if portrait['status'] == 'ready':
                break
            time.sleep(.15)
        else:
            raise AssertionError('The native Portrait & tile editor did not open')
        assert portrait['subjects'] == 1 and portrait['animations'], portrait
        invalid = call('session.presentation.configure', dict(target, revision=portrait['revision'],
            animation='missing-animation', tile_name='INVALID', target='card'), expected=False)
        assert invalid['code'] == 'animation_unavailable'
        assert call('session.presentation.inspect', target)['revision'] == portrait['revision']
        configured = call('session.presentation.configure', dict(target, revision=portrait['revision'],
            camera={'yaw': -.5, 'pitch': .15, 'zoom': 1.7, 'pan_y': .18}, tile_name='LIVE MARIO'))
        assert configured['tile_name'] == 'LIVE MARIO'
        assert abs(configured['camera']['zoom'] - 1.7) < .0001
        stale = call('session.presentation.capture', dict(target, revision=portrait['revision']), expected=False)
        assert stale['code'] == 'revision_conflict'
        def save_rendered_portrait(view):
            requested = call('session.presentation.capture', dict(target, revision=view['revision']))
            deadline = time.monotonic() + 30
            while time.monotonic() < deadline:
                result = call('session.presentation.inspect', target)
                if result.get('capture', {}).get('status') != 'pending':
                    assert result['capture']['id'] == requested['capture']['id']
                    assert result['capture']['status'] == 'saved', result
                    assert result['capture']['target'] == view['target']
                    return result
                time.sleep(.15)
            raise AssertionError('Portrait capture timed out')
        captured = save_rendered_portrait(configured)
        tile = call('presentation.read', saved)
        assert tile['tile_name'] == 'LIVE MARIO' and tile['revision'] == captured['saved_disk_revision'], tile
        assert tile['tile']['width'] == 464 and tile['tile']['height'] < 464
        assert hashlib.sha256((project / 'portrait.png').read_bytes()).hexdigest() == tile['tile']['sha256']
        assert abs(tile['tile']['framing']['pan_y'] - .18) < .0001
        card_view = call('session.presentation.configure', dict(target, revision=captured['revision'],
            target='card', camera={'yaw': .35, 'zoom': 1.3}))
        captured = save_rendered_portrait(card_view)
        portraits = call('presentation.read', saved)
        assert portraits['card']['width'] == 136 and portraits['card']['height'] == 188
        assert portraits['tile'] == tile['tile'] and portraits['tile_name'] == tile['tile_name']
        assert abs(portraits['card']['framing']['yaw'] - .35) < .0001
        assert call('fighters.installed', {'query': 'Live Mario'})['fighters'][0]['package_id'] == installed['package_id']
        malformed = output / 'bad-portrait.png'
        malformed.write_bytes(b'not a PNG')
        failed = call('presentation.edit', dict(saved, revision=portraits['revision'],
            tile_name='INVALID', image=str(malformed), target='card'), expected=False)
        assert failed['code'] == 'invalid_presentation'
        assert call('presentation.read', saved) == portraits
        edited = call('presentation.edit', dict(saved, revision=portraits['revision'], tile_name='MARIO LAB'))
        assert edited['saved'] and edited['revision'] != portraits['revision']
        stale_capture = call('session.presentation.capture', dict(target, revision=captured['revision']))
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline:
            captured = call('session.presentation.inspect', target)
            if captured['capture']['status'] != 'pending':
                break
            time.sleep(.15)
        assert captured['capture']['id'] == stale_capture['capture']['id']
        assert captured['capture']['status'] == 'failed' and 'draft changed' in captured['message'].lower()
        after_external = call('presentation.read', saved)
        assert after_external['revision'] == edited['revision'] and after_external['tile_name'] == 'MARIO LAB'
        assert after_external['tile'] == portraits['tile'] and after_external['card'] == portraits['card']
        result = call('session.presentation.close', dict(target, revision=captured['revision']))
        assert result['status'] == 'closed', result
        deadline = time.monotonic() + 90
        while time.monotonic() < deadline:
            reopened = call('session.inspect', target)
            if reopened['workshop_open'] and reopened['editing_allowed']:
                break
            time.sleep(.2)
        else:
            raise AssertionError('Could not return to the Workshop from Portrait & tile')
        installed = call('session.install', dict(target, revision=reopened['revision']))
        placed = call('session.presentation.place', dict(target, revision=installed['revision'],
            package_id=installed['installed_package_id']))
        assert placed['placed']
        assert call('session.presentation.place', dict(target, revision=placed['revision'],
            package_id=installed['installed_package_id']))['placed']
        (output / 'presentation.json').write_text(json.dumps(portraits, indent=2))
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
        print('PASS: live source/action authoring, undo, preview identity, practice, installation, exact export and native portrait capture')
    finally:
        if process and process.poll() is None:
            close_window(process.pid)
            process.wait(timeout=30)


if __name__ == '__main__':
    main()
