"""Inventory retail costume layouts and vertex sharing before weight editing.

Reads the user's GALE01 v1.02 ISO and writes counts and layout descriptions.
No costume assets are extracted and the game is not launched.
"""
import argparse
from collections import Counter, defaultdict
import json
import math
from pathlib import Path
import re
import struct
from audit_native_item_scripts import DatView
from prepare_native_assets import read_exact

def costumes(iso):
    with iso.open('rb') as file:
        header = read_exact(file, 0, 1088)
        if header[:6] != b'GALE01' or header[7] != 2:
            raise ValueError('Expected GALE01 v1.02')
        offset, size = struct.unpack_from('>II', header, 1060)
        fst = read_exact(file, offset, size)
        count = struct.unpack_from('>I', fst, 8)[0]
        names = fst[count * 12:]
        for i in range(1, count):
            name, at, size = struct.unpack_from('>III', fst, i * 12)
            if name >> 24:
                continue
            start = name & 16777215
            name = names[start:names.index(0, start)].decode()
            if re.fullmatch('Pl[A-Z][a-z][A-Z][a-z]\\.dat', name):
                yield (name, read_exact(file, at, size))

def inspect(payload):
    d = DatView(payload)
    raw = payload[32:32 + d.data_size]
    half = lambda at: struct.unpack_from('>H', raw, at)[0]
    real = lambda at: struct.unpack_from('>f', raw, at)[0]

    def ref(slot):
        kind, value = d.reference(slot)
        assert kind in ['local', 'null'], (slot, kind)
        return value if kind == 'local' else None
    roots = [at for name, at in d.roots.items() if name.endswith('_Share_joint')]
    if not roots:
        return dict(roots=list(d.roots), skip=True)
    stats = Counter()
    layouts = Counter()
    tex = defaultdict(set)
    seen_joints = set()
    maxima = Counter()
    position_arrays = set()

    def polygon(at, owner_flags, mesh_positions, mesh_indices, mesh_attributes):
        seen = set()
        while at is not None:
            assert at not in seen
            seen.add(at)
            flags = half(at + 12)
            kind = flags >> 12 & 3
            stats[f'polygons_type_{kind}'] += 1
            stats[f"owner_{('root' if owner_flags & 2 else 'deform' if owner_flags & 1 else 'helper')}_type_{kind}"] += 1
            attrs = []
            arrays = {}
            slot = ref(at + 8)
            while slot is not None and d.word(slot) != 255:
                attr, typ, cnt, fmt = struct.unpack_from('>4I', raw, slot)
                frac = raw[slot + 16]
                stride = half(slot + 18)
                if typ:
                    attrs.append((attr, typ, cnt, fmt, frac, stride))
                    if typ > 1:
                        arrays[attr] = ref(slot + 20)
                        assert arrays[attr] is not None
                slot += 24
            layouts[str(attrs)] += 1
            groups = []
            if kind == 2:
                table = ref(at + 20)
                for slot in range(10):
                    env = ref(table + slot * 4)
                    if env is None:
                        break
                    weights = []
                    while (bone := ref(env)) is not None:
                        weight = real(env + 4)
                        assert math.isfinite(weight) and weight >= 0
                        weights.append((bone, weight))
                        env += 8
                    assert weights and abs(sum((weight for _, weight in weights)) - 1) < 0.01
                    maxima['influences_per_envelope'] = max(maxima['influences_per_envelope'], len(weights))
                    if weights[0][1] >= 1:
                        stats['single_envelopes'] += 1
                        if owner_flags & 2:
                            stats['single_root_envelopes'] += 1
                        weights = [(weights[0][0], 1.0)]
                    merged = defaultdict(float)
                    for bone, weight in weights:
                        merged[bone] += weight
                    groups.append(tuple(sorted(merged.items())))
            elif kind == 0 and ref(at + 20) is not None:
                stats['shared_rigid_polygons'] += 1
            if kind != 1:
                attrs.sort(key=lambda a: 10 if a[0] == 25 else a[0])
                cursor = ref(at + 16)
                end = cursor + half(at + 14) * 32
                while cursor < end:
                    command = raw[cursor]
                    cursor += 1
                    if not command:
                        continue
                    stats[f'primitive_{command:02x}'] += 1
                    n = half(cursor)
                    cursor += 2
                    assert command in [128, 144, 152, 160], hex(command)
                    maxima['corners_per_primitive'] = max(maxima['corners_per_primitive'], n)
                    for _ in range(n):
                        position = None
                        matrix = 0
                        attribute_ids = []
                        for attr, typ, cnt, fmt, frac, stride in attrs:
                            if attr <= 8:
                                width = 1
                            elif attr in [11, 12]:
                                width = [2, 3, 4, 2, 3, 4][fmt]
                            else:
                                components = (3 if cnt else 2) if attr == 9 else (9 if cnt == 1 or (cnt == 2 and typ == 1) else 3) if attr in [10, 25] else 2 if cnt else 1
                                width = components * [1, 1, 2, 2, 4][fmt]
                            size = width if typ == 1 else 1 if typ == 2 else 2
                            if attr in [10, 25] and cnt == 2 and (typ != 1):
                                size *= 3
                            assert cursor + size <= end
                            index = raw[cursor] if typ == 2 else half(cursor) if typ == 3 else None
                            if typ > 1:
                                index_width = 1 if typ == 2 else 2
                                for component in range(3 if attr in [10, 25] and cnt == 2 else 1):
                                    component_at = cursor + component * index_width
                                    value = raw[component_at] if typ == 2 else half(component_at)
                                    assert arrays[attr] + value * stride + width <= len(raw), (attr, value, stride, width)
                                    attribute_ids.append((attr, arrays[attr], value))
                            if attr == 0:
                                assert raw[cursor] % 3 == 0
                                matrix = raw[cursor] // 3
                            elif attr == 9:
                                position = (arrays[attr], index) if typ > 1 else ('direct', cursor)
                                if typ > 1:
                                    position_arrays.add(arrays[attr])
                            if 1 <= attr <= 8:
                                tex[str(attr)].add(raw[cursor])
                            cursor += size
                        assert position is not None
                        if kind == 2:
                            assert matrix < len(groups), (matrix, len(groups))
                            binding = groups[matrix]
                        else:
                            binding = ('rigid', matrix)
                        mesh_positions[position].add(binding)
                        mesh_indices[position[1]].add(position[0])
                        mesh_attributes[position, binding].add(tuple(attribute_ids))
                        stats['corners'] += 1
                assert cursor == end, (cursor, end)
            at = ref(at + 4)

    def joint(at):
        while at is not None:
            assert at not in seen_joints
            seen_joints.add(at)
            flags = d.word(at + 4)
            if not flags & (32 | 16384):
                mesh = ref(at + 16)
                seen = set()
                while mesh is not None:
                    assert mesh not in seen
                    seen.add(mesh)
                    stats['meshes'] += 1
                    p = ref(mesh + 12)
                    mesh_positions = defaultdict(set)
                    mesh_indices = defaultdict(set)
                    mesh_attributes = defaultdict(set)
                    if p is not None:
                        polygon(p, flags, mesh_positions, mesh_indices, mesh_attributes)
                    conflicts = sum((len(bindings) > 1 for bindings in mesh_positions.values()))
                    aliases = sum((len(arrays) > 1 for arrays in mesh_indices.values()))
                    seams = sum((len(values) > 1 for values in mesh_attributes.values()))
                    stats['positions_with_multiple_bindings'] += conflicts
                    stats['indices_across_position_arrays'] += aliases
                    stats['positions_with_attribute_seams'] += seams
                    stats['meshes_with_multiple_bindings'] += bool(conflicts)
                    stats['meshes_with_index_aliases'] += bool(aliases)
                    stats['meshes_with_attribute_seams'] += bool(seams)
                    maxima['logical_vertices_per_mesh'] = max(maxima['logical_vertices_per_mesh'], sum(map(len, mesh_positions.values())))
                    mesh = ref(mesh + 4)
            child = ref(at + 8)
            if child is not None:
                joint(child)
            at = ref(at + 12)
    for root in roots:
        joint(root)
    return dict(stats=stats, layouts=layouts, maxima=maxima, position_arrays=len(position_arrays), texture_matrix_values={k: sorted(v) for k, v in tex.items()})

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--iso', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    results = {}
    totals = Counter()
    layouts = Counter()
    errors = {}
    maxima = Counter()
    for name, payload in costumes(args.iso):
        try:
            result = inspect(payload)
            results[name] = result
            if not result.get('skip'):
                totals.update(result['stats'])
                layouts.update(result['layouts'])
                for key, value in result['maxima'].items():
                    maxima[key] = max(maxima[key], value)
        except Exception as error:
            errors[name] = repr(error)
    document = dict(costumes=results, totals=totals, layouts=layouts, maxima=maxima, errors=errors)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(document, indent=2) + '\n')
    skipped = sum(bool(result.get('skip')) for result in results.values())
    print(json.dumps(dict(files=len(results), skipped=skipped, totals=totals,
                          layouts=len(layouts), maxima=maxima, errors=errors), indent=2))
    if not results or skipped or errors:
        raise SystemExit(1)
if __name__ == '__main__':
    main()
