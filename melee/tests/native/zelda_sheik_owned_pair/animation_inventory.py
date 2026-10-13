"""Compare physically packaged clips with their unchanged retail DAT records."""
import hashlib
import struct


def inventory(iso, project, archive, dat, root, family, common, common_root, extract):
    word = lambda data, offset: struct.unpack_from('>I', data, 32 + offset)[0]
    bank = (project / 'animation.dat').read_bytes()
    indexed = {}
    cursor = 0
    while cursor < len(bank):
        if bank[cursor] == 255:
            cursor += 1
            continue
        size, data_size, relocations, roots, externs = struct.unpack_from('>5I', bank, cursor)
        assert size >= 32 and cursor + size <= len(bank)
        blob = bank[cursor:cursor + size]
        table = 32 + data_size + 4 * relocations
        strings = table + 8 * (roots + externs)
        for i in range(roots):
            clip_root, name_offset = struct.unpack_from('>II', blob, table + i * 8)
            name_start = strings + name_offset
            name = blob[name_start:blob.index(0, name_start)].decode('ascii')
            assert name not in indexed
            indexed[name] = (cursor, clip_root, blob)
        cursor += size
    rows = []
    missing_demos = []
    for table_name, count, table in (('action', 311 if family == 19 else 317, word(dat, root + 12)),
                                   ('demo', 14, word(dat, root + 20))):
        for i in range(count):
            record = table + i * 24
            length = word(dat, record + 8)
            if not length:
                continue
            name_offset = word(dat, record)
            symbol = dat[32 + name_offset:dat.index(0, 32 + name_offset)].decode('ascii')
            tag = word(dat, record + 16) & 63
            if table_name == 'demo' and symbol not in indexed:
                missing_demos.append({'index': i, 'symbol': symbol, 'tag': tag, 'bytes': length})
                continue
            offset, clip_root, blob = indexed[symbol]
            retail, retail_root = extract(iso, archive, symbol)
            assert blob == retail and clip_root == retail_root and len(blob) == length
            rows.append({'table': table_name, 'index': i, 'symbol': symbol, 'tag': tag,
                         'bank_offset': offset, 'bytes': len(blob), 'root': clip_root,
                         'sha256': hashlib.sha256(blob).hexdigest()})
    foreign = [r for r in rows if r['tag'] != family]
    assert len(foreign) == 4 and {r['index'] for r in foreign} == {262, 263, 264, 265}
    assert all(r['tag'] == 33 and r['table'] == 'action' and r['symbol'].startswith('PlyTaro_Share_') for r in foreign)
    # Ft_Kind_None/Taro is an immutable common source skeleton, not a stock
    # fighter Definition or ftData dependency. The destination mapping is owned.
    parts_table = word(common, common_root + 16)
    descriptor = word(common, parts_table + 33 * 4)
    part_to_joint = word(common, descriptor + 4)
    joint_to_part = word(common, descriptor)
    part_count = word(common, descriptor + 8)
    return {'own_clips': len(rows) - len(foreign) + len(missing_demos), 'borrowed_clips': len(foreign),
            'physically_copied_clips': len(rows),
            'missing_demo_clips': missing_demos,
            'clips': rows, 'common_source_rig': {'kind': 33, 'parts': part_count,
                'part_to_joint': list(common[32 + part_to_joint:32 + part_to_joint + 54]),
                'joint_to_part': list(common[32 + joint_to_part:32 + joint_to_part + part_count])}}
