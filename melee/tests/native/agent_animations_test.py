"""Exercise animation authoring against real archives and native evaluation."""
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
    windows = Path(os.environ['SystemRoot'])
    with tempfile.TemporaryDirectory(prefix='Native animation agent ') as temp:
        root = Path(temp); data = root / 'userdata'; data.mkdir()
        env = dict(os.environ, LOCALAPPDATA=str(data), PATH=str(windows / 'System32') + ';' + str(windows))
        def run(*args, input=None):
            return subprocess.run([str(tool), *map(str, args)], input=input, text=True,
                capture_output=True, timeout=120, env=env, cwd=root, creationflags=subprocess.CREATE_NO_WINDOW)
        project = root / 'Animation Mario'
        cloned = run('clone-stock', iso, 'Mario', project, 'Animation Mario')
        assert cloned.returncode == 0, cloned.stderr
        target = {'project': str(project)}
        def call(name, args=None, expected=True):
            result = run('agent', 'call', name, '--stdin', input=json.dumps(args or {}))
            assert result.stdout, (name, result.returncode, result.stderr)
            reply = json.loads(result.stdout)
            assert reply['ok'] == expected, (name, reply)
            return reply['result'] if expected else reply['error']
        def read(view, clip=None, **args):
            if clip is not None: args['clip'] = clip
            return call('animations.read', dict(target, view=view, **args))
        def edit(operation, clip=None, **args):
            if clip is not None: args['clip'] = clip
            return call('animations.edit', dict(target, operation=operation,
                revision=call('project.inspect', target)['revision'], **args))
        def action(name):
            rows = call('actions.list', dict(target, query=name))['actions']
            return next(row['id'] for row in rows if row['name'] == name)
        def clip(name): return {'bank': 'action', 'index': action(name)}
        def track(ref, bone, channel):
            offset = 0
            while True:
                page = read('tracks', ref, offset=offset, limit=200)
                found = [row for row in page['tracks'] if row['bone'] == bone and row['channel'] == channel]
                if found: return found[0]['id']
                assert page['next_offset'] is not None, (bone, channel)
                offset = page['next_offset']
        def key(ref, t, frame):
            return next(row for row in read('keys', ref, track=t, limit=200)['keys'] if row['frame'] == frame)
        def sample(ref, t, frame):
            return read('samples', ref, track=t, frames=[frame])['samples'][0]['value']

        original = snapshot(project)
        clips = read('clips', limit=200)
        assert clips['total'] >= 303 and clips['next_offset'] == 200
        assert read('clips', offset=200, limit=200)['clips']
        bones = read('bones', limit=200)['bones']
        assert len(bones) > 20 and all(b['parent'] is None or b['parent'] < b['id'] for b in bones)
        original_clip = clip('AttackAirLw')
        original_info = read('info', original_clip)
        original_keys = read('keys', original_clip, track=0, limit=200)['keys']
        read('pose', original_clip, frame=0, limit=200)
        assert snapshot(project) == original
        source = clip('AttackAirN'); destination = clip('AttackAirB')
        source_events = call('actions.read', dict(target, action_id=source['index'], view='timeline', limit=200))['occurrences']
        created = edit('create', target_action=source['index'], name='AgentRest', frames=20)
        assert created['saved'] and not created['installed'] and created['clip'] == source
        assert read('bones', source, limit=200)['rig_bound']
        hip = next(b['id'] for b in bones if b.get('role') == 'HipN'); second = hip + 1
        inserted = edit('insert', source, insertions=[
            {'bone': hip, 'channel': 'rotation_z', 'frame': 10, 'value': 1.5},
            {'bone': second, 'channel': 'rotation_z', 'frame': 10, 'value': 2.5},
            {'bone': 1, 'channel': 'position_x', 'frame': 10, 'value': 5}])
        t1 = track(source, hip, 'rotation_z'); t2 = track(source, second, 'rotation_z')
        assert sample(source, t1, 10) == 1.5 and sample(source, t2, 10) == 2.5
        assert read('pose', source, frame=10, limit=200)['pose'][hip]['rotation'][2] == 1.5
        kept = snapshot(project)
        bad = call('animations.edit', dict(target, operation='insert', clip=source,
            revision=inserted['revision'], insertions=[
                {'bone': hip, 'channel': 'rotation_z', 'frame': 5, 'value': 9},
                {'bone': 511, 'channel': 'rotation_z', 'frame': 5, 'value': 9}]), False)
        assert bad['code'] == 'animation_edit_rejected' and snapshot(project) == kept
        assert call('animations.edit', dict(target, operation='duration', clip=source, frames=10,
            revision=inserted['revision']), False)['code'] == 'animation_edit_rejected'
        assert snapshot(project) == kept
        before_events = call('actions.read', dict(target, view='timeline', action_id=destination['index'], limit=200))['occurrences']
        flags = call('actions.read', dict(target, view='metadata', action_id=destination['index']))['animation_flags']
        call('actions.edit', dict(target, operation='playback', action_id=destination['index'],
            revision=call('project.inspect', target)['revision'], animation_flags=(flags & ~63) | 1 | 0xc0000000))
        assert not read('bones', destination, limit=1)['rig_bound']
        assert call('animations.read', dict(target, clip=destination, view='pose', frame=0), False)['code'] == 'foreign_clip_binding'
        edit('duplicate', source, target_action=destination['index'], name='AgentDuplicate')
        info = read('info', destination)
        assert info['binding_flags'] & 63 == read('info', source)['binding_flags'] & 63
        assert info['binding_flags'] & 0xc0000000 == 0xc0000000
        assert call('actions.read', dict(target, view='timeline', action_id=destination['index'], limit=200))['occurrences'] == before_events
        copied = edit('copy_pose', destination, source_clip=source, source_frame=10, frame=2, bones=[hip, second])
        assert copied['written_channels'] >= 2 and copied['skipped_channels'] == 0
        assert sample(destination, track(destination, hip, 'rotation_z'), 2) == 1.5
        assert sample(source, t1, 2) != 1.5
        changed = edit('change', source, keys=[
            {'track': t1, 'key': key(source, t1, 10)['id'], 'frame': 12, 'value': 3},
            {'track': t2, 'key': key(source, t2, 10)['id'], 'frame': 12, 'value': 4}])
        assert changed['changed_keys'] == 2 and [row['frame'] for row in changed['applied_keys']] == [12, 12]
        assert sample(source, t1, 12) == 3
        kept = snapshot(project)
        bad = call('animations.edit', dict(target, operation='change', clip=source, revision=changed['revision'], keys=[
            {'track': t1, 'key': key(source, t1, 12)['id'], 'value': 77},
            {'track': t2, 'key': key(source, t2, 12)['id'], 'tangent': 77}]), False)
        assert bad['code'] == 'animation_edit_rejected' and snapshot(project) == kept
        transformed = edit('transform', source, selection={'bones': [hip, second], 'channels': ['rotation_z'], 'start_frame': 12, 'end_frame': 12},
            transform={'frame_offset': 1, 'value_scale': .5})
        assert transformed['changed_keys'] == 2
        assert key(source, t1, 13)['value'] == 1.5 and key(source, t2, 13)['value'] == 2
        rotated = edit('transform', source, keys=[{'track': t1, 'key': key(source, t1, 13)['id']}],
            transform={'pivot_frame': 13, 'rotation': .7853981633974483})
        assert rotated['applied_keys'][0]['frame'] == 12
        assert abs(rotated['applied_keys'][0]['value'] - 1.0606601718) < 1e-5
        deleted = edit('delete', source, selection={'bones': [second], 'channels': ['rotation_z']})
        assert deleted['deleted_keys'] >= 3
        assert not any(row['bone'] == second and row['channel'] == 'rotation_z' for row in read('tracks', source, limit=200)['tracks'])
        edit('delete', source, selection={'start_frame': 11})
        edit('duration', source, frames=10)
        assert read('info', source)['frames'] == 10 and read('info', destination)['frames'] == 20
        assert read('info', original_clip)['frames'] == original_info['frames']
        assert read('keys', original_clip, track=0, limit=200)['keys'] == original_keys
        assert call('actions.read', dict(target, action_id=source['index'], view='timeline', limit=200))['occurrences'] == source_events
        native = {'bank': 'action', 'index': 295}
        quantized = edit('change', native, keys=[{'track': 0, 'key': 0, 'value': .251}])
        assert quantized['requested_keys'][0]['value'] == .251
        assert abs(quantized['applied_keys'][0]['value'] - .251) < .00013
        assert quantized['applied_keys'][0]['value'] != .251
        before = sample(native, 0, 1)
        tangent = edit('change', native, keys=[{'track': 0, 'key': 0, 'tangent': .25}])
        assert tangent['applied_keys'][0]['tangent'] == .25 and sample(native, 0, 1) != before
        edit('change', native, keys=[{'track': 0, 'key': 1, 'frame': 3}])
        joined = read('keys', native, track=0)['keys']
        assert joined[1]['frame'] == joined[2]['frame'] == 3
        edit('transform', native, keys=[{'track': 0, 'key': 1}, {'track': 0, 'key': 2}],
            transform={'pivot_frame': 3, 'rotation': .1})
        joined = read('keys', native, track=0)['keys']
        assert joined[1]['frame'] == joined[2]['frame'] == 3
        assert call('scripts.check', target)['accepted']
        print('PASS: animation discovery, native evaluation, independent clips, bulk keys/transforms, pose copy and atomic rejection')


if __name__ == '__main__': main()
