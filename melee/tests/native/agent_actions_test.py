"""Exercise native action authoring in batches using only the packaged CLI."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import struct
import sys
import tempfile


def snapshot(root):
    return {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in root.rglob('*') if p.is_file()}


def action_table(data):
    _, size, relocations, roots, externals = struct.unpack_from('>5I', data)
    symbols = 32 + size + 4 * relocations
    strings = symbols + 8 * (roots + externals)
    root = next(offset for offset, name in
        (struct.unpack_from('>2I', data, symbols + 8 * i) for i in range(roots))
        if data[strings+name:].split(b'\0')[0].startswith(b'ftData'))
    return struct.unpack_from('>I', data, 32 + root + 12)[0]


def main():
    tool, iso = map(lambda p: Path(p).resolve(), sys.argv[1:3])
    with tempfile.TemporaryDirectory(prefix='native action authoring ') as temporary:
        root = Path(temporary)
        appdata = root / 'userdata'
        appdata.mkdir()
        windows = Path(os.environ['SystemRoot'])
        env = dict(os.environ, LOCALAPPDATA=str(appdata),
                   PATH=str(windows / 'System32') + ';' + str(windows))
        def run(*parts, input=None):
            return subprocess.run([str(tool), *map(str, parts)], input=input, env=env, cwd=root,
                capture_output=True, encoding='utf-8', timeout=60, creationflags=subprocess.CREATE_NO_WINDOW)
        def call(method, args, expected=True):
            result = run('agent', 'call', method, '--stdin', input=json.dumps(args))
            assert result.stdout, (method, result.returncode, result.stderr)
            value = json.loads(result.stdout)
            assert value['ok'] == expected, (method, value)
            return value['result'] if expected else value['error']
        project = root / 'Action Mario'
        cloned = run('clone-stock', iso, 'Mario', project, 'Action Mario')
        assert cloned.returncode == 0, (cloned.stdout, cloned.stderr)
        target = {'project': str(project)}
        def read(view, **args): return call('actions.read', dict(target, view=view, **args))
        def edit(operation, **args):
            return call('actions.edit', dict(target, revision=call('project.inspect', target)['revision'], operation=operation, **args))
        def action(name):
            candidates = call('actions.list', dict(target, query=name, limit=200))['actions']
            exact = [row for row in candidates if row['name'] == name]
            assert len(exact) == 1, candidates
            return exact[0]['id']
        dair = action('AttackAirLw')
        assert read('metadata', action_id=dair)['timeline_complete']
        original_timeline = read('timeline', action_id=dair, limit=200)['occurrences']
        assert [o['frame'] for o in original_timeline] == sorted(o['frame'] for o in original_timeline)
        kinds = read('types', limit=200)['types']
        hitbox_kind = next(row for row in kinds if row['kind'] == 'hitbox')
        assert len(hitbox_kind['fields']) >= 28
        assert hitbox_kind['fields']['element']['choices'][2]['label'] == 'Electric'
        assert hitbox_kind['fields']['angle']['maximum'] == 511
        owned = edit('own_hitboxes', action_id=dair)
        assert owned['converted_hitboxes'] == 3, owned
        events = read('events', action_id=dair, limit=200)['events']
        assert events and all(not row['timer'] for row in events)
        hits = [row for row in events if row['kind'] == 'hitbox']
        assert len(hits) == 3, hits
        original = read('event', action_id=dair, event_id=hits[0]['event_id'])
        assert len(original['frames']) >= 4, original
        assert original['fields']['id']['readOnly'] and not original['fields']['damage']['readOnly']
        revision = call('project.inspect', target)['revision']
        changed = edit('update', action_id=dair, event_id=hits[0]['event_id'], properties={
            'damage': 11, 'element': 2, 'knockback_growth': 122, 'shield_damage': -5,
            'radius': 4.25, 'offset_x': 2, 'hit_grounded': True, 'hit_aerial': False,
            'refresh_hit_eligibility': False})
        assert changed['changed'] and changed['saved'] and not changed['installed']
        current = read('event', action_id=dair, event_id=changed['event_id'])
        assert current['properties']['damage'] == 11 and current['properties']['element'] == 2
        assert current['properties']['shield_damage'] == -5 and not current['properties']['hit_aerial']
        assert abs(current['properties']['radius'] - 4.25) <= current['fields']['radius']['quantum']
        assert current['frames'] == original['frames']
        kept = snapshot(project)
        rejected = call('actions.edit', dict(target, revision=revision, operation='update', action_id=dair,
            event_id=changed['event_id'], properties={'damage': 8}), expected=False)
        assert rejected['code'] == 'revision_conflict' and snapshot(project) == kept
        for properties in ({'damage': 7, 'gfx_id': 123}, {'damage': 1024}, {'id': 3}):
            rejected = call('actions.edit', dict(target, revision=changed['revision'], operation='update', action_id=dair,
                event_id=changed['event_id'], properties=properties), expected=False)
            assert rejected['code'] in ('invalid_arguments', 'action_edit_rejected'), rejected
            assert snapshot(project) == kept
        # A repeated sound moves as one authored event. The hitbox pattern and
        # every repetition count remain unchanged.
        sounds = [row for row in read('events', action_id=dair, limit=200)['events'] if row['kind'] == 'sound']
        assert sounds, events
        sound = read('event', action_id=dair, event_id=sounds[0]['event_id'])
        hit_frames = sorted(tuple(read('event', action_id=dair, event_id=row['event_id'])['frames'])
                            for row in read('events', action_id=dair, limit=200)['events'] if row['kind'] == 'hitbox')
        moved = edit('timing', action_id=dair, event_id=sounds[0]['event_id'], delta=1)
        after = read('event', action_id=dair, event_id=moved['event_id'])
        assert after['frames'] == [frame + 1 for frame in sound['frames']]
        assert sorted(tuple(read('event', action_id=dair, event_id=row['event_id'])['frames'])
                      for row in read('events', action_id=dair, limit=200)['events'] if row['kind'] == 'hitbox') == hit_frames
        kept = snapshot(project)
        assert call('actions.edit', dict(target, revision=moved['revision'], operation='timing', action_id=dair,
            event_id=moved['event_id'], delta=100), expected=False)['code'] == 'action_edit_rejected'
        assert snapshot(project) == kept
        # A new event can create the first script on an action with no source.
        all_actions = call('actions.list', dict(target, limit=200))['actions']
        # Stock Mario points even his inactive actions at an End script. Make
        # one genuine null reference to exercise first-script creation too.
        empty = all_actions[0]['id']
        path = project / 'fighter.dat'
        data = bytearray(path.read_bytes())
        slot = action_table(data) + empty * 24 + 12
        struct.pack_into('>I', data, 32 + slot, 0)
        _, data_size, relocations = struct.unpack_from('>3I', data)
        relocation = next(i for i in range(relocations)
            if struct.unpack_from('>I', data, 32 + data_size + 4 * i)[0] == slot)
        del data[32 + data_size + 4 * relocation:32 + data_size + 4 * relocation + 4]
        struct.pack_into('>I', data, 0, len(data))
        struct.pack_into('>I', data, 8, relocations - 1)
        path.write_bytes(data)
        empty_flags = read('metadata', action_id=empty)
        assert empty_flags['command_count'] == 0
        edit('playback', action_id=empty, loop=not empty_flags['loop'])
        assert read('metadata', action_id=empty)['loop'] != empty_flags['loop']
        edit('playback', action_id=empty, animation_flags=empty_flags['animation_flags'])
        defaults = dict(hitbox_kind['properties'], id=0, duration=2, damage=9, angle=45, element=1)
        created = edit('create', action_id=empty, frame=3, kind='hitbox', properties=defaults)
        new = read('event', action_id=empty, event_id=created['event_id'])
        assert new['frames'] == [3] and new['properties']['duration'] == 2 and new['properties']['damage'] == 9
        retimed = edit('timing', action_id=empty, event_id=created['event_id'], frame=5, duration=3)
        new = read('event', action_id=empty, event_id=retimed['event_id'])
        assert new['frames'] == [5] and new['properties']['duration'] == 3
        timeline = read('timeline', action_id=empty, limit=200)
        assert timeline['timeline_complete'] and timeline['occurrences'][0]['duration_end_frame'] == 8
        kept = snapshot(project)
        assert call('actions.edit', dict(target, revision=retimed['revision'], operation='create', action_id=empty,
            frame=6, kind='effect', properties={'damage': 5}), expected=False)['code'] == 'action_edit_rejected'
        assert snapshot(project) == kept, 'Failed creation wrote a partial event'
        # Generic command codecs also create an event absent from a donor move.
        wind = edit('create', action_id=empty, frame=9, kind='opcode_58', properties={'field_0': 0, 'field_1': 0})
        assert read('event', action_id=empty, event_id=wind['event_id'])['opcode'] == 58
        edit('delete', action_id=empty, event_id=wind['event_id'])
        # Deletion changes opaque handles, so inspect the action again.
        remaining = read('events', action_id=empty, limit=200)['events']
        hit = next(row for row in remaining if row['kind'] == 'hitbox')
        edit('delete', action_id=empty, event_id=hit['event_id'])
        assert read('events', action_id=empty)['total'] == 0
        visibility = edit('create', action_id=empty, frame=0, kind='opcode_31', properties={'group': 0, 'variant': -1})
        assert read('event', action_id=empty, event_id=visibility['event_id'])['properties']['variant'] == -1
        edit('delete', action_id=empty, event_id=visibility['event_id'])
        throw_type = next(row for row in kinds if row['kind'] == 'throw')
        assert throw_type['fields']['slot']['maximum'] == 1
        kept = snapshot(project)
        assert call('actions.edit', dict(target, revision=call('project.inspect', target)['revision'],
            operation='create', action_id=empty, frame=0, kind='throw', properties={'slot': 2}), expected=False)['code'] == 'action_edit_rejected'
        assert snapshot(project) == kept
        flags = read('metadata', action_id=dair)['animation_flags']
        edit('playback', action_id=dair, loop=True, root_motion=True)
        meta = read('metadata', action_id=dair)
        assert meta['loop'] and meta['root_motion']
        kept = snapshot(project)
        assert call('actions.edit', dict(target, revision=meta['revision'], operation='playback', action_id=dair,
            animation_flags=meta['animation_flags'] ^ 0x00400000), expected=False)['code'] == 'action_edit_rejected'
        assert snapshot(project) == kept
        edit('playback', action_id=dair, animation_flags=flags)
        assert call('scripts.check', target)['accepted']
        # Member selection changes only that member's archive.
        family = root / 'Action Zelda'
        result = run('clone-stock', iso, 'Zelda', family, 'Action Zelda')
        assert result.returncode == 0, result.stderr
        result = run('clone-stock', iso, 'Sheik', family / 'partner', 'Action Sheik')
        assert result.returncode == 0, result.stderr
        (family / 'family.txt').write_text('mode=alternate\nprimary_key=main\npartner_key=partner\npartner=partner\n')
        other = {'project': str(family)}
        members = call('project.inspect', other)['members']
        assert len(members) == 2
        other['member'] = members[1]['key']
        chosen = next(row['id'] for row in call('actions.list', dict(other, query='AttackAirN'))['actions'] if row['name'] == 'AttackAirN')
        event = next(row for row in call('actions.read', dict(other, view='events', action_id=chosen))['events'] if row['kind'] == 'hitbox')
        main_hash = hashlib.sha256((family / 'fighter.dat').read_bytes()).digest()
        old = call('project.inspect', other)['revision']
        assert call('actions.edit', dict(other, revision=old, operation='update', action_id=chosen,
            event_id=event['event_id'], properties={'damage': 17}))['changed']
        assert hashlib.sha256((family / 'fighter.dat').read_bytes()).digest() == main_hash
        # Effect attachment uses this rig's size, including indices beyond
        # Mario's 61 joints. The old effect setter hardcoded Mario's limit.
        if members[1]['part_slots'] > 61:
            effect = call('actions.edit', dict(other, revision=call('project.inspect', other)['revision'],
                operation='create', action_id=chosen, frame=0, kind='effect', properties={'bone': 61, 'gfx_id': 0}))
            viewed = call('actions.read', dict(other, action_id=chosen, view='event', event_id=effect['event_id']))
            assert viewed['properties']['bone'] == 61
        # A past absolute wait executes now, while later relative waits still
        # include its negative timer debt (lbcommand.c Command_01/Command_02).
        fixture = root / 'Past wait'
        result = run('clone-stock', iso, 'Mario', fixture, 'Past wait')
        assert result.returncode == 0, result.stderr
        path = fixture / 'fighter.dat'
        data = bytearray(path.read_bytes())
        table = action_table(data)
        start = struct.unpack_from('>I', data, 32 + table + dair * 24 + 12)[0]
        words = [1 << 26 | 5, 2 << 26 | 3, 19 << 26 | 1,
                 1 << 26 | 1, 19 << 26 | 2, 1 << 26 | 2, 19 << 26 | 3, 0]
        struct.pack_into('>8I', data, 32 + start, *words)
        path.write_bytes(data)
        trace = call('actions.read', dict(project=str(fixture), action_id=dair, view='timeline'))
        assert [o['frame'] for o in trace['occurrences']] == [5, 5, 6], trace
        print('PASS: typed action fields, atomic rejection, repeated timing, direct event creation, duration, deletion, playback, timer debt and paired member isolation')


if __name__ == '__main__': main()
