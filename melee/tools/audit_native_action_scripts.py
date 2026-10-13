"""Preflight every fighter action script in the retail Melee character DATs.

This checks the source bytecode graph before converting it for the native
client. It does not execute commands or prove gameplay fidelity.
"""

import argparse
from collections import Counter
from pathlib import Path
import re
import struct

from prepare_native_assets import read_exact


# Widths for opcodes 10..58, from ftAction_803C0870 in ftaction.c.
ACTION_WIDTHS = (
    5, 5, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 7, 4, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 2, 1, 4,
)
assert len(ACTION_WIDTHS) == 49


def disc_fighters(iso: Path):
    with iso.open("rb") as source:
        header = read_exact(source, 0, 0x440)
        if header[:6] != b"GALE01" or header[7] != 2:
            raise ValueError("Expected the USA v1.02 Melee ISO")
        fst_offset, fst_size = struct.unpack_from(">II", header, 0x424)
        fst = read_exact(source, fst_offset, fst_size)
        count = struct.unpack_from(">I", fst, 8)[0]
        if count * 12 > len(fst):
            raise ValueError("Truncated disc file table")
        names = fst[count * 12:]
        for index in range(1, count):
            name, offset, size = struct.unpack_from(">III", fst, index * 12)
            if name >> 24:
                continue
            start = name & 0xFFFFFF
            filename = names[start:names.index(0, start)].decode("ascii")
            if re.fullmatch(r"Pl[A-Za-z]{2}\.dat", filename) and filename != "PlCo.dat":
                yield filename, read_exact(source, offset, size)


def read_u32(member: bytes, data_size: int, offset: int) -> int:
    if offset < 0 or offset + 4 > data_size:
        raise ValueError(f"DAT word 0x{offset:x} is out of range")
    return struct.unpack_from(">I", member, 32 + offset)[0]


def fighter_root(member: bytes) -> tuple[int, int, set[int], set[int]]:
    if len(member) < 32:
        raise ValueError("Truncated DAT header")
    size, data_size, reloc_count, root_count, extern_count = struct.unpack_from(
        ">5I", member
    )
    table = 32 + data_size + reloc_count * 4
    strings = table + (root_count + extern_count) * 8
    if size > len(member) or strings > size:
        raise ValueError("Truncated DAT tables")
    relocations = {
        struct.unpack_from(">I", member, 32 + data_size + index * 4)[0]
        for index in range(reloc_count)
    }
    targets = {read_u32(member, data_size, slot) for slot in relocations}
    targets.add(data_size)
    roots = []
    for index in range(root_count):
        root, name = struct.unpack_from(">II", member, table + index * 8)
        label_start = strings + name
        if label_start >= size:
            raise ValueError("DAT root name is out of range")
        label = member[label_start:member.index(0, label_start)].decode("ascii")
        if label.startswith("ftData"):
            roots.append(root)
    if len(roots) != 1:
        raise ValueError(f"Expected one fighter data root, found {len(roots)}")
    return data_size, roots[0], relocations, targets


def audit_fighter(member: bytes) -> tuple[int, int, int, Counter, int]:
    data_size, root, relocations, targets = fighter_root(member)
    table_slot = root + 12
    if table_slot not in relocations:
        raise ValueError("Fighter wait table is not relocated")
    wait = read_u32(member, data_size, table_slot)
    targets.add(root)
    end = min(target for target in targets if target > wait)
    if (end - wait) % 24:
        raise ValueError("Fighter wait table has a partial record")
    state_count = (end - wait) // 24
    if not 200 <= state_count <= 512:
        raise ValueError(f"Implausible fighter wait count: {state_count}")

    script_entries = []
    for index in range(state_count):
        slot = wait + index * 24 + 12
        if slot in relocations:
            script_entries.append(read_u32(member, data_size, slot))
        elif read_u32(member, data_size, slot) != 0:
            raise ValueError(f"Wait script {index} is not relocated")

    visited = set()
    payload_words = set()
    pending = list(set(script_entries))
    counts = Counter()
    null_branches = 0
    while pending:
        at = pending.pop()
        while at not in visited:
            if at in payload_words:
                raise ValueError(f"Script jumps into payload at 0x{at:x}")
            word = read_u32(member, data_size, at)
            opcode = word >> 26
            if opcode >= 10 + len(ACTION_WIDTHS):
                raise ValueError(f"Unknown action opcode {opcode} at 0x{at:x}")
            width = (2 if opcode in (5, 7) else 1) if opcode < 10 else ACTION_WIDTHS[opcode - 10]
            if at + width * 4 > data_size:
                raise ValueError(f"Truncated opcode {opcode} at 0x{at:x}")
            visited.add(at)
            for payload in range(at + 4, at + width * 4, 4):
                if payload in visited or payload in payload_words:
                    raise ValueError(f"Overlapping payload at 0x{payload:x}")
                payload_words.add(payload)
            counts[opcode] += 1
            if opcode in (5, 7):
                operand = at + 4
                if operand in relocations:
                    pending.append(read_u32(member, data_size, operand))
                elif read_u32(member, data_size, operand) == 0:
                    null_branches += 1
                else:
                    raise ValueError(f"Unrelocated branch at 0x{at:x}")
            if opcode in (0, 6, 7):
                break
            at += width * 4
    return state_count, len(script_entries), len(visited), counts, null_branches


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", required=True, type=Path)
    args = parser.parse_args()

    archives = 0
    states = entries = instructions = null_branches = 0
    total_opcodes = Counter()
    for name, member in disc_fighters(args.iso):
        archive_states, archive_entries, archive_instructions, opcodes, nulls = audit_fighter(member)
        print(f"{name}: {archive_states} states, {archive_entries} scripts, "
              f"{archive_instructions} instruction starts, {nulls} null branches")
        archives += 1
        states += archive_states
        entries += archive_entries
        instructions += archive_instructions
        null_branches += nulls
        total_opcodes.update(opcodes)
    if archives != 33:
        raise ValueError(f"Expected 33 fighter archives, found {archives}")
    print(f"TOTAL: {archives} archives, {states} states, {entries} scripts, "
          f"{instructions} instruction starts, {null_branches} null branches")
    print("OPCODES:", " ".join(f"{op}:{count}" for op, count in sorted(total_opcodes.items())))


if __name__ == "__main__":
    main()
