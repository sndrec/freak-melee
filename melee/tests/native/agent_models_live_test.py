"""Verify model authoring and whole-asset history in the actual hidden Workshop."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import time

from agent_live_test import close_window, snapshot
from agent_models_test import model_checks


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'tool', 'iso', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    root = args.output.resolve(); root.mkdir(parents=True, exist_ok=False)
    profile = root / 'userdata'; profile.mkdir()
    windows = Path(os.environ['SystemRoot'])
    env = dict(os.environ, LOCALAPPDATA=str(profile),
        PATH=str(windows / 'System32') + ';' + str(windows), FREAK_MELEE_MENU_MOTION='0')
    for service in ('CHAT', 'SERVICE', 'COMMUNITY'):
        env['FREAK_MELEE_' + service + '_ENDPOINT'] = 'http://127.0.0.1:1'
    def call(method, arguments=None, expected=True):
        result = subprocess.run([str(args.tool.resolve()), 'agent', 'call', method, '--stdin'],
            input=json.dumps(arguments or {}), env=env, cwd=root, capture_output=True,
            encoding='utf-8', timeout=90, creationflags=subprocess.CREATE_NO_WINDOW)
        assert result.stdout, (method, result.returncode, result.stderr)
        reply = json.loads(result.stdout)
        assert reply['ok'] == expected, (method, arguments, reply)
        return reply['result'] if expected else reply['error']
    stock = next(row for row in call('stock.list', {'query': 'Mario'})['fighters'] if row['name'] == 'Mario')
    project = profile / 'Freak Melee/projects/Model Mario'
    call('project.clone_stock', dict(fighter_id=stock['fighter_id'], iso=str(args.iso.resolve()),
        output=str(project), title='Model Mario'))
    content = profile / 'Freak Melee/content'; content.mkdir(parents=True, exist_ok=True)
    (content / 'startup.txt').write_text('12 0 1\n\n')
    (content / 'hub-profile.txt').write_text('Agent model review\n\n')
    baseline = snapshot(project)
    process = None
    try:
        with (root / 'stdout.log').open('w') as out, (root / 'stderr.log').open('w') as err:
            process = subprocess.Popen([str(args.exe.resolve()), '--iso', str(args.iso.resolve()), '--hidden', '--mute'],
                cwd=root, env=env, stdout=out, stderr=err, creationflags=subprocess.CREATE_NO_WINDOW)
        (root / 'pid.txt').write_text(str(process.pid))
        deadline = time.monotonic() + 120
        while time.monotonic() < deadline:
            assert process.poll() is None, ('Game exited', process.returncode, root)
            matches = [row for row in call('instances.list')['instances'] if row['process_id'] == process.pid and row['available']]
            if matches: break
            time.sleep(.2)
        else: raise AssertionError('No live endpoint')
        target = {'instance': matches[0]['instance']}
        state = call('session.inspect', target)
        call('session.open', dict(target, project=str(project), revision=state['revision']))
        def settled():
            deadline = time.monotonic() + 120
            while time.monotonic() < deadline:
                assert process.poll() is None, ('Game exited', process.returncode, root)
                state = call('session.inspect', target)
                if not state['preview_loading'] and state['editing_allowed'] and state['preview_scene'] == 'practice':
                    return state
                time.sleep(.15)
            raise AssertionError(('Preview did not settle', state))
        settled()
        result = model_checks(call, target, project, live=True, settled=settled)
        state = settled()
        assert state['dirty'] and snapshot(project) == baseline
        saved = call('session.save', dict(target, revision=state['revision']))
        assert saved['saved'] and not saved['dirty'] and snapshot(project) != baseline
        loaded = call('models.read', dict(project=str(project), view='info'))
        assert loaded['meshes'] == result['meshes']
        built = call('session.build', dict(target, revision=saved['revision']))
        assert built['package_id']
        # History remains valid after Save/build; changing the source identity
        # must not cause labels, embedded images or size-changing data to be lost.
        call('session.undo', dict(target, revision=built['revision']))
        state = settled()
        assert state['dirty']
        call('session.redo', dict(target, revision=state['revision']))
        state = settled()
        assert not state['dirty']
        (root / 'result.json').write_text(json.dumps(dict(status='PASS', **result), indent=2))
        print('PASS: real model edits, full-asset Undo/Redo, private live ownership, preview reload and saved build')
    finally:
        if process and process.poll() is None:
            close_window(process.pid)
            process.wait(timeout=30)


if __name__ == '__main__': main()
