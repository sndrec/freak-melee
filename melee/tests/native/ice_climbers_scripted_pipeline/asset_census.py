"""Independent package animation record/bank census; no constructor substitute."""
import hashlib


def animation_census(dat, bank, root, own_kind):
    rows, demo_records = [], []
    own = foreign = 0
    for label, count, table in (('action', 321, dat.word(root + 12)),
                                ('demo', 14, dat.word(root + 20))):
        for index in range(count):
            record = table + index * 24
            size = dat.word(record + 8)
            if not size:
                continue
            symbol = dat.text(dat.word(record))
            tag = dat.word(record + 16) & 63
            own += tag == own_kind
            foreign += tag != own_kind
            row = {'table': label, 'index': index, 'symbol': symbol, 'tag': tag, 'bytes': size}
            if label == 'demo':
                # Demo offsets address their separately loaded archives. A
                # reused action symbol is not evidence the demo clip was decoded
                # from this action bank or that its original offset was rebuilt.
                row.update(original_offset=dat.word(record + 4),
                           same_name_in_action_bank=symbol in bank,
                           payload_status='undecoded separate-demo metadata')
                demo_records.append(row)
                continue
            assert symbol in bank, ('Missing physical action clip', row)
            offset, root_offset, blob = bank[symbol]
            assert dat.word(record + 4) == offset and len(blob) == size, row
            row.update(bank_offset=offset, root_offset=root_offset,
                       sha256=hashlib.sha256(blob).hexdigest())
            rows.append(row)
    borrowed = [row for row in rows if row['tag'] != own_kind]
    assert len(borrowed) == foreign == 4
    assert {row['index'] for row in borrowed} == {262, 263, 264, 265}
    assert all(row['table'] == 'action' and row['tag'] == 33 and
               row['symbol'].startswith('PlyTaro_Share_') for row in borrowed)
    return {'own_clips': own, 'borrowed_clips': foreign, 'physical_clips': len(rows),
            'clips': rows, 'undecoded_demo_records': demo_records}
