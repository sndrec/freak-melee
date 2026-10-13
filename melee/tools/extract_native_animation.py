"""Extract a named fighter clip from an original Melee ISO for native inspection."""
import argparse
from pathlib import Path
import struct

from prepare_native_assets import read_exact


def extract_clip(iso, archive, symbol):
    with iso.open('rb') as source:
        header = read_exact(source, 0, 0x440)
        if header[:6] != b'GALE01' or header[7] != 2:
            raise ValueError('Expected USA v1.02 Melee ISO')
        fst_offset, fst_size = struct.unpack_from('>II', header, 0x424)
        fst = read_exact(source, fst_offset, fst_size)
        count = struct.unpack_from('>I', fst, 8)[0]
        if count * 12 > len(fst):
            raise ValueError('Truncated disc file table')
        names = fst[count * 12:]
        payload = None
        for index in range(1, count):
            name, offset, size = struct.unpack_from('>III', fst, index * 12)
            if name >> 24:
                continue
            start = name & 0xFFFFFF
            entry = names[start:names.index(0, start)].decode('ascii')
            if entry == archive:
                payload = read_exact(source, offset, size)
                break
        if payload is None:
            raise ValueError(f'Archive not found: {archive}')
    cursor = 0
    while cursor + 32 <= len(payload):
        size, data_size, relocs, roots, externs = struct.unpack_from('>5I', payload, cursor)
        if size < 32 or cursor + size > len(payload):
            raise ValueError('Invalid animation archive member size')
        member = payload[cursor:cursor + size]
        table = 32 + data_size + 4 * relocs
        strings = table + 8 * (roots + externs)
        if strings > size:
            raise ValueError('Invalid animation member tables')
        for index in range(roots):
            root, name = struct.unpack_from('>II', member, table + 8 * index)
            start = strings + name
            entry = member[start:member.index(0, start)].decode('ascii')
            if entry == symbol:
                return member, root
        cursor = (cursor + size + 31) & ~31
    raise ValueError(f'Clip not found: {symbol}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--iso', required=True, type=Path)
    parser.add_argument('--archive', required=True)
    parser.add_argument('--symbol', required=True)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    data, root = extract_clip(args.iso, args.archive, args.symbol)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    print(f'{args.output}: {len(data)} bytes, root 0x{root:x}')
