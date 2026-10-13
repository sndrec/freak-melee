"""Validate retail color-script instruction boundaries and branch relocations.

The bytecode is still GameCube data. Passing this preflight does not mean the
Windows runtime can execute it; it defines the corpus its native port must cover.
"""

import argparse
from collections import Counter
from pathlib import Path
import struct

from extract_native_animation import extract_clip


WIDTH = {opcode: 1 for opcode in range(24)}
WIDTH.update({5: 2, 7: 2, 13: 2, 14: 2, 15: 2, 18: 2,
              19: 2, 21: 5, 22: 3})
TERMINAL = {0, 6, 7, 10}


def read_u32(member: bytes, data_size: int, offset: int) -> int:
    if offset < 0 or offset + 4 > data_size:
        raise ValueError(f"Word 0x{offset:x} is outside DAT data")
    return struct.unpack_from(">I", member, 32 + offset)[0]


def audit(member: bytes, table: int, count: int) -> tuple[int, int, Counter]:
    if len(member) < 32:
        raise ValueError("DAT header is truncated")
    _, data_size, reloc_count, _, _ = struct.unpack_from(">5I", member)
    if 32 + data_size + reloc_count * 4 > len(member):
        raise ValueError("DAT relocation table is truncated")
    relocations = {
        struct.unpack_from(">I", member, 32 + data_size + i * 4)[0]
        for i in range(reloc_count)
    }
    if table + count * 8 > data_size:
        raise ValueError("Color entry table is outside DAT data")

    starts = set()
    non_null = 0
    for index in range(count):
        slot = table + index * 8
        if slot in relocations:
            target = read_u32(member, data_size, slot)
            if target >= data_size or target % 4:
                raise ValueError(f"Entry {index} has an invalid script target")
            starts.add(target)
            non_null += 1
        elif read_u32(member, data_size, slot) != 0:
            raise ValueError(f"Entry {index} has no script relocation")

    counts = Counter()
    visited = set()
    payload_words = set()
    pending = list(starts)
    while pending:
        at = pending.pop()
        while at not in visited:
            if at in payload_words:
                raise ValueError(f"Branch or entry lands in payload at 0x{at:x}")
            visited.add(at)
            word = read_u32(member, data_size, at)
            opcode = word >> 26
            if opcode not in WIDTH:
                raise ValueError(f"Invalid color opcode {opcode} at 0x{at:x}")
            width = WIDTH[opcode]
            if at + width * 4 > data_size:
                raise ValueError(f"Truncated opcode {opcode} at 0x{at:x}")
            for payload in range(at + 4, at + width * 4, 4):
                if payload in visited or payload in payload_words:
                    raise ValueError(f"Overlapping opcode payload at 0x{payload:x}")
                payload_words.add(payload)
            counts[opcode] += 1
            if opcode in (5, 7):
                target_slot = at + 4
                if target_slot not in relocations:
                    raise ValueError(f"Branch at 0x{at:x} has no relocation")
                target = read_u32(member, data_size, target_slot)
                if target >= data_size or target % 4:
                    raise ValueError(f"Branch at 0x{at:x} has invalid target")
                pending.append(target)
            if opcode in TERMINAL:
                break
            at += width * 4
    return non_null, len(visited), counts


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", required=True, type=Path)
    args = parser.parse_args()

    for archive, symbol, root_slot, count in (
        ("PlCo.dat", "ftLoadCommonData", 6, 123),
        ("PlCo.dat", "ftLoadCommonData", 7, 6),
        ("LbBf.dat", "lbBgFlashColAnimData", None, 16),
        ("ItCo.usd", "itPublicData", 5, 7),
    ):
        member, root = extract_clip(args.iso, archive, symbol)
        data_size = struct.unpack_from(">I", member, 4)[0]
        table = root if root_slot is None else read_u32(
            member, data_size, root + root_slot * 4
        )
        non_null, instructions, counts = audit(member, table, count)
        print(f"{archive} root {root_slot if root_slot is not None else symbol}: "
              f"{non_null}/{count} entries, {instructions} instruction starts")
        print("  opcodes:", " ".join(f"{op}:{n}" for op, n in sorted(counts.items())))


if __name__ == "__main__":
    main()
