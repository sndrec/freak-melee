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
        manual_hit = next(row for row in read('events', action_id=dair)['events']
                          if row['kind'] == 'hitbox')
        filtered = edit('update', action_id=dair, event_id=manual_hit['event_id'],
                        properties={'exclude_grabbed': True, 'only_hit_grabbed': False})
        filtered = read('event', action_id=dair, event_id=filtered['event_id'])
        assert filtered['opcode'] == 62 and filtered['properties']['launch_mode'] == 0
        assert not filtered['properties']['timed'] and filtered['properties']['exclude_grabbed']
        manual = edit('update', action_id=dair, event_id=filtered['event_id'],
                      properties={'launch_mode': 1, 'launch_up': 10, 'damage': 1.55})
        updated = read('event', action_id=dair, event_id=manual['event_id'])
        assert updated['opcode'] == 62 and not updated['properties']['timed']
        assert abs(updated['properties']['damage'] - 1.55) < 1e-6
        owned = edit('own_hitboxes', action_id=dair)
        assert owned['converted_hitboxes'] == 3, owned
        events = read('events', action_id=dair, limit=200)['events']
        assert events and all(not row['timer'] for row in events)
        hits = [row for row in events if row['kind'] == 'hitbox']
        assert len(hits) == 3, hits
        original = read('event', action_id=dair, event_id=hits[0]['event_id'])
        assert original['opcode'] == 61 and original['properties']['launch_up'] == 10
        assert original['properties']['damage'] == updated['properties']['damage']
        assert original['properties']['exclude_grabbed']
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
        hit_ids = [row['event_id'] for row in read('events', action_id=dair)['events'] if row['kind'] == 'hitbox']
        batch = [{'event_id': event_id, 'properties': {'damage': 12 + i, 'radius': 3.14159}}
                 for i, event_id in enumerate(hit_ids)]
        grouped = edit('update', action_id=dair, updates=batch)
        assert len(grouped['updated_events']) == 3
        for i, event in enumerate(grouped['updated_events']):
            assert event['previous_event_id'] == hit_ids[i] and event['requested']['radius'] == 3.14159
            assert event['properties']['damage'] == 12 + i
            assert event['first_frame'] == original['frames'][0] and event['occurrences'] == len(original['frames'])
            assert abs(event['properties']['radius'] - 3.14159) <= event['fields']['radius']['quantum']
        kept = snapshot(project)
        fresh_hits = [row['event_id'] for row in read('events', action_id=dair)['events'] if row['kind'] == 'hitbox']
        for bad_properties in ({'damage': 1024}, {'gfx_id': 123}):
            invalid = [{'event_id': fresh_hits[0], 'properties': {'damage': 77}},
                       {'event_id': fresh_hits[1], 'properties': bad_properties}]
            rejected = call('actions.edit', dict(target, revision=grouped['revision'], operation='update', action_id=dair, updates=invalid), expected=False)
            assert rejected['code'] in ('action_edit_rejected', 'invalid_arguments') and snapshot(project) == kept
        duplicate = [{'event_id': fresh_hits[0], 'properties': {'damage': n}} for n in (1, 2)]
        rejected = call('actions.edit', dict(target, revision=grouped['revision'], operation='update', action_id=dair, updates=duplicate), expected=False)
        assert rejected['code'] == 'duplicate_event' and snapshot(project) == kept
        launched = edit('update', action_id=dair, updates=[
            {'event_id': fresh_hits[1], 'properties': {'launch_mode': 1,
                'launch_forward': -2.5, 'launch_up': 11.75, 'launch_frames': 5.5}},
            {'event_id': fresh_hits[2], 'properties': {'launch_mode': 2,
                'launch_momentum': 0.875, 'launch_attraction': 0.125,
                'launch_min_up': -0.5, 'launch_ground_angle': 77.25,
                'launch_override_ground_knockback': True, 'launch_ground_growth': 40,
                'launch_ground_base': 50}},
        ])
        for event, mode in zip(launched['updated_events'], (1, 2)):
            value = read('event', action_id=dair, event_id=event['event_id'])
            assert value['opcode'] == 61 and value['properties']['launch_mode'] == mode
            assert value['frames'] == original['frames']
            for name, expected in event['requested'].items():
                assert value['properties'][name] == expected
        target_event = launched['updated_events'][0]['event_id']
        kept = snapshot(project)
        rejected = call('actions.edit', dict(target, revision=launched['revision'],
            operation='update', action_id=dair, event_id=target_event,
            properties={'launch_frames': 0.5}), expected=False)
        assert rejected['code'] in ('invalid_arguments', 'action_edit_rejected'), rejected
        assert snapshot(project) == kept
        restored = edit('update', action_id=dair, event_id=target_event,
                        properties={'launch_mode': 0})
        value = read('event', action_id=dair, event_id=restored['event_id'])
        assert value['opcode'] == 59 and value['frames'] == original['frames']
        fighter_hash = hashlib.sha256((project / 'fighter.dat').read_bytes()).digest()
        named = edit('metadata', action_id=dair, label='Orbital kicks', hidden=True)
        metadata = read('metadata', action_id=dair)
        assert metadata['label'] == 'Orbital kicks' and metadata['hidden']
        assert hashlib.sha256((project / 'fighter.dat').read_bytes()).digest() == fighter_hash
        assert call('actions.list', dict(target, query='Orbital kicks'))['actions'][0]['hidden']
        kept = snapshot(project)
        assert not edit('metadata', action_id=dair, label='Orbital kicks', hidden=True)['changed']
        assert snapshot(project) == kept
        rejected = call('actions.edit', dict(target, revision=named['revision'], operation='metadata', action_id=dair,
            label='Invalid\nlabel', hidden=False), expected=False)
        assert rejected['code'] == 'action_edit_rejected' and snapshot(project) == kept
        edit('metadata', action_id=dair, label='', hidden=False)
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
        defaults = dict(hitbox_kind['properties'], id=0, duration=2, damage=9.25,
                        angle=45, element=1)
        created = edit('create', action_id=empty, frame=3, kind='hitbox', properties=defaults)
        new = read('event', action_id=empty, event_id=created['event_id'])
        assert new['frames'] == [3] and new['properties']['duration'] == 2
        assert new['properties']['damage'] == 9.25 and new['properties']['launch_mode'] == 0
        capsule = edit('update', action_id=empty, event_id=created['event_id'], properties={
            'capsule': True, 'offset_x': 11.5, 'offset_y': 3.25, 'offset_z': -2,
            'end_x': 11.5, 'end_y': 3.25, 'end_z': -2})
        capsule = capsule['updated_events'][0]
        assert capsule['properties']['capsule']
        for axis, expected in zip('xyz', (11.5, 3.25, -2)):
            start = capsule['properties']['offset_' + axis]
            end = capsule['properties']['end_' + axis]
            assert start == end and abs(end - expected) < .002
        retimed = edit('timing', action_id=empty, event_id=capsule['event_id'], frame=5, duration=3)
        new = read('event', action_id=empty, event_id=retimed['event_id'])
        assert new['frames'] == [5] and new['properties']['duration'] == 3
        assert new['opcode'] == 61 and new['properties']['damage'] == 9.25
        assert new['properties']['capsule']
        assert new['properties']['end_z'] == capsule['properties']['end_z']
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
        assert throw_type['fields']['damage']['type'] == 'number'
        kept = snapshot(project)
        assert call('actions.edit', dict(target, revision=call('project.inspect', target)['revision'],
            operation='create', action_id=empty, frame=0, kind='throw', properties={'slot': 2}), expected=False)['code'] == 'action_edit_rejected'
        assert snapshot(project) == kept
        throw = edit('create', action_id=empty, frame=0, kind='throw', properties={
            'slot': 1, 'damage': 7.5, 'angle': 44, 'knockback_growth': 70})
        record = read('event', action_id=empty, event_id=throw['event_id'])
        assert record['opcode'] == 63 and record['kind'] == 'throw'
        assert record['properties']['damage'] == 7.5 and record['properties']['angle'] == 44
        edit('create', action_id=empty, frame=12, kind='release_throw')
        events = read('events', action_id=empty)['events']
        throw = next(row for row in events if row['kind'] == 'throw')
        kept = snapshot(project)
        assert call('actions.edit', dict(target, revision=call('project.inspect', target)['revision'],
            operation='update', action_id=empty, event_id=throw['event_id'],
            properties={'damage': -0.5}), expected=False)['code'] == 'action_edit_rejected'
        assert snapshot(project) == kept
        changed = edit('update', action_id=empty, event_id=throw['event_id'],
                       properties={'damage': 8, 'base_knockback': 60})['updated_events'][0]
        assert changed['opcode'] == 34 and changed['properties']['damage'] == 8
        assert changed['properties']['base_knockback'] == 60
        timeline = read('timeline', action_id=empty)
        assert any(row['opcode'] == 20 and row['frame'] == 12 for row in timeline['occurrences'])
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
        main_names = (family / 'workshop_names.txt').read_bytes() if (family / 'workshop_names.txt').exists() else None
        call('actions.edit', dict(other, revision=call('project.inspect', other)['revision'],
            action_id=chosen, operation='metadata', label='Partner aerial', hidden=True))
        assert call('actions.read', dict(other, action_id=chosen, view='metadata'))['label'] == 'Partner aerial'
        assert (family / 'partner/workshop_names.txt').exists()
        assert ((family / 'workshop_names.txt').read_bytes() if (family / 'workshop_names.txt').exists() else None) == main_names
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
        # Sharing an entire script forces first-update localization. Every
        # batch handle still addresses its original event after that copy.
        shared = root / 'Shared script'
        result = run('clone-stock', iso, 'Mario', shared, 'Shared script')
        assert result.returncode == 0, result.stderr
        path = shared / 'fighter.dat'
        data = bytearray(path.read_bytes())
        table = action_table(data)
        source = struct.unpack_from('>I', data, 32 + table + dair * 24 + 12)[0]
        struct.pack_into('>I', data, 32 + table + (dair - 1) * 24 + 12, source)
        path.write_bytes(data)
        copied = {'project': str(shared)}
        original_events = call('actions.read', dict(copied, action_id=dair, view='events'))['events']
        hits = [row['event_id'] for row in original_events if row['kind'] == 'hitbox']
        assert len(hits) == 3
        original = [call('actions.read', dict(copied, action_id=dair - 1, view='event', event_id=h))['properties'] for h in hits]
        updates = [{'event_id': h, 'properties': {'damage': 30 + i}} for i, h in enumerate(hits)]
        changed = call('actions.edit', dict(copied, action_id=dair, operation='update', updates=updates,
            revision=call('project.inspect', copied)['revision']))
        assert [row['properties']['damage'] for row in changed['updated_events']] == [30, 31, 32]
        assert [call('actions.read', dict(copied, action_id=dair - 1, view='event', event_id=h))['properties'] for h in hits] == original
        print('PASS: typed action fields, atomic rejection, repeated timing, direct event creation, duration, deletion, playback, timer debt and paired member isolation')


if __name__ == '__main__': main()
