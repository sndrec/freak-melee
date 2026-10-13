"""Verify Samus's direction-vector loads directly in the supplied US 1.02 ISO."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

START, END = 0x802BA3BC, 0x802BA5DC
# The distance helper writes dir to stack 0x28. Retail's clamp uses those
# values directly; the decompiled C's uninitialized dir_ptr is not retail UB.
WORDS = {
    0x802BA4F4: (0x38A10028, 'addi r5,r1,0x28: direction output'),
    0x802BA4F8: (0x4BFE97A1, 'bl 0x802A3C98: calculate direction'),
    0x802BA508: (0xC0210028, 'lfs f1,0x28(r1): direction X'),
    0x802BA514: (0xD01B0014, 'stfs f0,0x14(r27): link position X'),
    0x802BA518: (0xC041002C, 'lfs f2,0x2C(r1): direction Y'),
    0x802BA528: (0xD01B0018, 'stfs f0,0x18(r27): link position Y'),
    0x802BA52C: (0xC0410030, 'lfs f2,0x30(r1): direction Z'),
    0x802BA53C: (0xD01B001C, 'stfs f0,0x1C(r27): link position Z'),
}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--iso', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    with args.iso.open('rb') as file:
        file.seek(0x420)
        dol_offset = struct.unpack('>I', file.read(4))[0]
        file.seek(dol_offset)
        header = file.read(0x100)
        offsets = struct.unpack('>18I', header[:0x48])
        addresses = struct.unpack('>18I', header[0x48:0x90])
        sizes = struct.unpack('>18I', header[0x90:0xD8])
        file.seek(dol_offset)
        dol = file.read(max(offset + size for offset, size in zip(offsets, sizes)))
    dol_sha1 = hashlib.sha1(dol).hexdigest()
    if dol_sha1 != '08e0bf20134dfcb260699671004527b2d6bb1a45':
        raise RuntimeError('Expected retail US 1.02 DOL; found ' + dol_sha1)
    body = None
    for offset, address, size in zip(offsets, addresses, sizes):
        if address <= START < END <= address + size:
            body = dol[offset + START - address:offset + END - address]
            break
    if body is None:
        raise RuntimeError('Samus pull routine is not mapped')
    for address, (word, _) in WORDS.items():
        actual = struct.unpack_from('>I', body, address - START)[0]
        if actual != word:
            raise RuntimeError(f'Unexpected retail instruction at {address:08X}: {actual:08X}')
    receipt = {
        'status': 'PASS', 'iso': str(args.iso.resolve()),
        'dol_sha1': dol_sha1, 'function': 'it_802BA3BC',
        'function_address': f'{START:08X}',
        'function_sha256': hashlib.sha256(body).hexdigest(),
        'verified_instructions': {f'{address:08X}': label for address, (_, label) in WORDS.items()},
        'conclusion': 'Retail reads the calculated stack direction directly; no uninitialized direction pointer is used.',
        'limits': 'Static retail instruction evidence, not emulated execution or native gameplay acceptance.',
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt))


if __name__ == '__main__':
    main()
