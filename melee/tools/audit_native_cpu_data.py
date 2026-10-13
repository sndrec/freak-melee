"""Inventory the retail PlCo.dat CPU behavior graph before native conversion.

The source calls this root Fighter_804D64FC. The audit validates all fixed
tables, command-script terminations and attack-record arrays. It does not
execute CPU decisions or claim they work in a native match.
"""

import argparse
from bisect import bisect_right
from pathlib import Path
import struct

from audit_native_item_scripts import DatView
from extract_native_animation import extract_clip


SCRIPT_COUNT = 62
FIGHTER_COUNT = 32
ATTACK_FAMILIES = 7
ATTACK_RECORD_SIZE = 36


def local(dat: DatView, slot: int) -> int:
    kind, target = dat.reference(slot)
    if kind != "local":
        raise ValueError(f"Expected local CPU pointer at 0x{slot:x}: {kind}")
    return target


def next_target(targets: list[int], source: int) -> int:
    return targets[bisect_right(targets, source)]


def audit(iso: Path):
    member, _ = extract_clip(iso, "PlCo.dat", "ftLoadCommonData")
    dat = DatView(member)
    root = dat.roots["ftLoadCommonData"]
    cpu = local(dat, root + 22 * 4)
    targets = sorted({0, dat.data_size, *dat.roots.values(),
                      *(dat.word(slot) for slot in dat.relocations)})
    if (next_target(targets, cpu) - cpu != 12 * 4 or
            dat.word(cpu + 40) != 0 or dat.word(cpu + 44) != 0):
        raise ValueError("CPU descriptor is not ten pointers and two zero words")
    tables = [local(dat, cpu + i * 4) for i in range(10)]
    expected_sizes = [SCRIPT_COUNT * 4] + [FIGHTER_COUNT * 4] * 8 + [6 * 4]
    for index, (table, size) in enumerate(zip(tables, expected_sizes)):
        if next_target(targets, table) - table != size:
            raise ValueError(f"CPU table {index} size differs from {size}")

    if dat.reference(tables[0])[0] != "null":
        raise ValueError("CPU command script slot zero is not empty")
    script_sources = [local(dat, tables[0] + i * 4)
                      for i in range(1, SCRIPT_COUNT)]
    if len(set(script_sources)) != len(script_sources):
        raise ValueError("CPU command scripts unexpectedly alias")
    script_commands = script_bytes = 0
    for source in script_sources:
        at = source
        end = next_target(targets, source)
        while at < end:
            command = member[32 + at]
            width = 1 if command <= 0x7F else 2 if command <= 0xBF else 3
            if at + width > end:
                raise ValueError(f"Truncated CPU command at 0x{at:x}")
            script_commands += 1
            at += width
            if command == 0x7F:
                script_bytes += at - source
                break
        else:
            raise ValueError(f"Unterminated CPU script at 0x{source:x}")

    attack_records = 0
    for family in range(ATTACK_FAMILIES):
        table = tables[family + 1]
        family_records = 0
        for fighter in range(FIGHTER_COUNT):
            source = local(dat, table + fighter * 4)
            end = next_target(targets, source)
            for count in range(33):
                at = source + count * ATTACK_RECORD_SIZE
                if at + ATTACK_RECORD_SIZE > end:
                    raise ValueError(f"Unterminated CPU attack array at 0x{source:x}")
                command = dat.word(at)
                if command == 0:
                    family_records += count
                    break
            else:
                raise ValueError(f"CPU attack array exceeds 32 entries at 0x{source:x}")
        attack_records += family_records
        print(f"attack family {family}: {family_records} records")

    for index, count in ((8, FIGHTER_COUNT), (9, 6)):
        for i in range(count):
            value = struct.unpack_from(">f", member, 32 + tables[index] + i * 4)[0]
            if not (-100000.0 < value < 100000.0):
                raise ValueError(f"Invalid CPU scalar at table {index}[{i}]")
    observed = (len(script_sources), script_bytes, script_commands,
                ATTACK_FAMILIES * FIGHTER_COUNT, attack_records)
    expected = (61, 1158, 652, 224, 1136)
    if observed != expected:
        raise ValueError(f"CPU graph inventory {observed} != {expected}")
    print(f"TOTAL: {observed[0]} scripts, {observed[1]} script bytes, "
          f"{observed[2]} commands, {observed[3]} fighter attack arrays, "
          f"{observed[4]} attack records, 32 thresholds, 6 reach values")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", required=True, type=Path)
    args = parser.parse_args()
    audit(args.iso)


if __name__ == "__main__":
    main()
