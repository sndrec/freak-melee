"""Run a seeded Workshop test in an isolated hidden and muted client."""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import subprocess
import struct
import time

from agent_live_test import close_window, snapshot


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for field in ('exe', 'tool', 'iso', 'output'):
        parser.add_argument('--' + field, required=True, type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True)
    exe, tool, iso = (path.resolve() for path in (args.exe, args.tool, args.iso))
    appdata = output / 'userdata'
    content = appdata / 'Freak Melee/content'
    content.mkdir(parents=True)
    (content / 'startup.txt').write_text('12 0 1\n\n')
    (content / 'hub-profile.txt').write_text('Practice review\n\n')
    windows = Path(os.environ['SystemRoot'])
    env = dict(os.environ, LOCALAPPDATA=str(appdata), PATH=f'{windows / "System32"};{windows}')
    for service in ('CHAT', 'SERVICE', 'COMMUNITY'):
        env[f'FREAK_MELEE_{service}_ENDPOINT'] = 'http://127.0.0.1:1'
    env['FREAK_MELEE_MENU_MOTION'] = '0'
    no_window = subprocess.CREATE_NO_WINDOW

    def call(tool_name, arguments=None, error=None):
        command = subprocess.run([str(tool), 'agent', 'call', tool_name, '--stdin'],
            cwd=output, env=env, input=json.dumps(arguments or {}), encoding='utf-8',
            capture_output=True, creationflags=no_window, timeout=90)
        assert command.stdout, (tool_name, command.stderr)
        result = json.loads(command.stdout)
        if error:
            assert not result['ok'] and result['error']['code'] == error, (tool_name, result)
            return result['error']
        assert result['ok'], (tool_name, result)
        return result['result']

    def stock(name):
        return next(row['fighter_id'] for row in call('stock.list', {'query': name})['fighters']
                    if row['name'] == name)

    project = appdata / 'Freak Melee/projects/Practice Mario'
    call('project.clone_stock', {'iso': str(iso), 'fighter_id': stock('Mario'),
        'output': str(project), 'title': 'Practice Mario'})
    saved = {'project': str(project)}
    sources = call('scripts.list', saved)
    fireball_file = next(row['file'] for row in sources['files'] if row['file'].endswith('_Fireball.fms'))
    state_file = next(row['file'] for row in sources['files'] if row['file'] == 'fighter_state.fms')
    state_source = call('scripts.read', dict(saved, file=state_file))['text']
    fireball_source = call('scripts.read', dict(saved, file=fireball_file))['text']
    call('scripts.update', dict(saved, revision=sources['revision'], files=[
        {'file': state_file, 'text': state_source + '\nstate int checkpoint_probe\n'},
        {'file': fireball_file, 'text': fireball_source.replace('    on special_n:\n',
            '    on special_n:\n'
            '        state.checkpoint_probe = state.checkpoint_probe + 1\n'
            '        if state.checkpoint_probe > 1:\n'
            '            state.checkpoint_probe = 1 / (state.checkpoint_probe - 2)\n', 1)},
    ]))
    opponent = stock('Samus')
    disk = snapshot(project)
    process = None
    try:
        with (output / 'stdout.log').open('w') as stdout, (output / 'stderr.log').open('w') as stderr:
            process = subprocess.Popen([str(exe), '--iso', str(iso), '--hidden', '--mute'],
                cwd=output, env=env, stdout=stdout, stderr=stderr, creationflags=no_window)
        (output / 'pid.txt').write_text(str(process.pid))

        def wait(read, done, seconds=90):
            deadline = time.monotonic() + seconds
            while time.monotonic() < deadline:
                assert process.poll() is None, f'Game exited with {process.returncode}'
                observed = read()
                if done(observed):
                    return observed
                assert not observed or observed.get('status') != 'failed', observed
                time.sleep(.12)
            raise AssertionError(f'Timed out at {observed}')

        def discover():
            return next((row for row in call('instances.list')['instances']
                         if row['process_id'] == process.pid and row['available']), None)

        instance = wait(discover, bool)
        assert instance['build_matches'] and Path(instance['executable']) == exe
        target = {'instance': instance['instance']}

        def workshop():
            return call('session.inspect', target)

        def trial():
            result = call('session.practice.state', target)
            result.pop('request_id', None)
            return result

        call('session.open', dict(target, revision=workshop()['revision'], project=str(project)))
        editor = wait(workshop, lambda state: state['editing_allowed'] and state['preview_ready'])
        started = call('session.practice.start', dict(target, revision=editor['revision'], controlled={
            'seed': 0x12345678, 'player': {'spawn': 2, 'facing': -1},
            'opponents': [{'port': 1, 'fighter_id': opponent, 'spawn': 1, 'facing': 1, 'damage': 11}],
            'sandbag': False}))
        ready = wait(trial, lambda state: state['status'] == 'ready')
        assert ready['frame'] == 0 and ready['test_id'] == started['test_id']
        assert ready['seed'] == 0x12345678 and ready['fighter_revision'] == editor['fighter_revision']
        assert ready['package_id'] == started['package_id']

        def selected(state):
            return dict(target, test_id=state['test_id'], test_revision=state['test_revision'])

        def trace(state, begin, limit=120):
            return call('session.practice.trace', dict(selected(state), **{'from': begin}, limit=limit))['frames']

        first = trace(ready, 0, 1)
        protagonist, defender = first[0]['fighters']
        assert (protagonist['port'], protagonist['x'], protagonist['facing']) == (0, 20, -1), first
        assert (defender['port'], defender['x'], defender['damage']) == (1, -20, 11), first
        time.sleep(.4)
        idle = trial()
        assert idle == ready and trace(ready, 0, 1) == first, (ready, idle)

        def run(state, segments, capture=None):
            end = state['frame'] + sum(segment['frames'] for segment in segments)
            arguments = dict(selected(state), inputs=segments)
            if capture:
                arguments['capture'] = capture
            started = call('session.practice.run', arguments)
            assert started['target_frame'] == end and started['status'] == 'running'
            next_state = wait(trial, lambda value: value['status'] == 'ready')
            assert next_state['frame'] == end
            return (next_state, started['capture_id']) if capture else next_state

        step = run(ready, [{'frames': 1}])
        assert step['frame'] == 1 and [row['frame'] for row in trace(step, 0, 2)] == [0, 1]
        call('session.practice.run', dict(selected(ready), inputs=[{'frames': 1}]), 'practice_conflict')
        warmed = run(step, [{'frames': 179}])
        loaded = workshop()
        assert loaded['loaded_preview_package_id'] == warmed['package_id']
        assert loaded['preview_fighter_revision'] == warmed['fighter_revision']
        warming = trace(warmed, 176)
        assert warming[-1]['frame'] == 180 and warming[-1]['fighters'][0]['facing'] == -1
        call('session.practice.checkpoint', dict(selected(warmed), action='save'))
        checkpoint = wait(trial, lambda value: value['status'] == 'ready')
        assert checkpoint['checkpoint_frame'] == 180
        fireball = [{'frames': 1, 'pads': [{'port': 0, 'buttons': ['B']}]}, {'frames': 99}]
        finished = run(checkpoint, fireball)
        first_run = trace(finished, 180)
        assert any(row['owned_items'] for row in first_run), first_run[-2:]
        assert first_run[0]['fighters'][0]['script_state'] != first_run[-1]['fighters'][0]['script_state']
        assert first_run[-1]['fighters'][1]['damage'] > 11, first_run[-2:]
        assert first_run[-1]['owned_items'] == 0, first_run[-2:]
        observed = [hit for row in first_run for hit in row['item_hits']]
        assert len(observed) == 1 and observed[0]['attacker_actor'] == 0 and observed[0]['victim_actor'] == 2
        checks = [
            {'predicate': 'item_hit_count', 'port': 1, 'item_kind': observed[0]['item_kind'],
             'from': 181, 'through': 280, 'expected': 1},
            {'predicate': 'facing', 'port': 0, 'from': 181, 'through': 280, 'expected': -1},
            {'predicate': 'hitbox_inactive', 'port': 0, 'from': 260, 'through': 280},
            {'predicate': 'no_script_faults'},
        ]
        result = call('session.practice.assert', dict(selected(finished), checks=checks))
        assert result['passed'], result
        wrong = call('session.practice.assert', dict(selected(finished), checks=[dict(checks[0], expected=2)]))
        assert not wrong['passed'] and wrong['checks'][0]['observed'] == 1
        faulted = run(finished, fireball)
        faults = [fault for row in trace(faulted, 281) for fault in row['script_faults']]
        assert len(faults) == 1 and faults[0]['source_kind'] == 'fighter' and faults[0]['line'] > 0, faults
        fault_check = call('session.practice.assert', dict(selected(faulted), checks=[checks[3]]))
        assert not fault_check['passed'] and fault_check['checks'][0]['first_failure_frame'] > 280
        call('session.practice.checkpoint', dict(selected(faulted), action='restore'))
        restored = wait(trial, lambda value: value['status'] == 'ready')
        assert restored['frame'] == 180
        png = output / 'practice.png'
        second, capture_id = run(restored, fireball, {'scope': 'viewport', 'output': str(png)})
        second_run = trace(second, 180)
        (output / 'traces.json').write_text(json.dumps([first_run, second_run], indent=2))
        assert first_run == second_run
        receipt = wait(lambda: call('session.preview.capture_status', dict(target, capture_id=capture_id)),
                       lambda result: result['status'] == 'ready')
        image = png.read_bytes()
        assert image[:8] == b'\x89PNG\r\n\x1a\n'
        assert struct.unpack('>II', image[16:24]) == (receipt['width'], receipt['height'])
        assert receipt['png_sha256'] == hashlib.sha256(image).hexdigest()
        assert receipt['view']['practice_frame'] == second['frame']
        assert receipt['view']['fighter_revision'] == second['fighter_revision']
        assert receipt['view']['package_id'] == second['package_id']
        mcp = subprocess.run([str(tool), 'mcp'], input='\n'.join(map(json.dumps, [
            {'jsonrpc': '2.0', 'id': 1, 'method': 'initialize', 'params': {
                'protocolVersion': '2025-11-25', 'capabilities': {},
                'clientInfo': {'name': 'Workshop practice acceptance', 'version': '1'}}},
            {'jsonrpc': '2.0', 'method': 'notifications/initialized'},
            {'jsonrpc': '2.0', 'id': 2, 'method': 'tools/call', 'params': {
                'name': 'session.preview.capture_status', 'arguments': dict(target, capture_id=capture_id)}}])) + '\n',
            cwd=output, env=env, encoding='utf-8', capture_output=True, creationflags=no_window, timeout=15)
        assert mcp.returncode == 0, mcp.stderr
        result = json.loads(mcp.stdout.splitlines()[-1])['result']
        assert any(block['type'] == 'image' and base64.b64decode(block['data']) == image
                   for block in result['content'])
        (output / 'capture.json').write_text(json.dumps(receipt, indent=2))
        pending = call('session.practice.reset', selected(second))
        assert pending['status'] == 'checkpoint_pending'
        reset = wait(trial, lambda value: value['status'] == 'ready')
        assert reset['frame'] == 0 and trace(reset, 0, 1) == first
        moves = call('session.actions.list', dict(target, query='AttackAirLw'))['actions']
        clip = next(row['id'] for row in moves if row['name'] == 'AttackAirLw')
        aerial = run(reset, [
            {'frames': 180},
            {'frames': 1, 'pads': [{'port': 0, 'buttons': ['X']}]},
            {'frames': 4},
            {'frames': 1, 'pads': [{'port': 0, 'buttons': ['A'], 'stick_y': -127}]},
            {'frames': 27},
        ])
        attack = next(fighter for row in trace(aerial, 180) for fighter in row['fighters']
                      if fighter['port'] == 0 and fighter['animation_id'] == clip and fighter['hitboxes'])
        assert attack['action_id'] != attack['animation_id'], attack
        assert snapshot(project) == disk
        state = workshop()
        call('session.practice.stop', dict(target, revision=state['revision']))
        wait(workshop, lambda value: value['editing_allowed'] and value['preview_ready'] and not value['testing'])
        assert trial()['status'] == 'closed'
        assert snapshot(project) == disk
        (output / 'result.json').write_text(json.dumps({'status': 'PASS', 'test': reset}, indent=2))
        print('PASS: seeded practice, exact stepping, item-hit assertions, script faults, persistent state and identical checkpoint replay')
    finally:
        if process and process.poll() is None:
            close_window(process.pid)
            process.wait(timeout=30)


if __name__ == '__main__':
    main()
