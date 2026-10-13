"""Inventory a retail DAT root's immediate pointer table without relocating it.

This is a source-data preflight for native importers, not a descriptor decoder.
Offsets and spans refer to the DAT data section (after the 32-byte header).
"""

import argparse
from pathlib import Path
import struct

from extract_native_animation import extract_clip


def inventory(
    member: bytes, root: int, count: int, stride: int
) -> list[tuple[int, int | None, int, int]]:
    if len(member) < 32:
        raise ValueError("DAT header is truncated")
    _, data_size, reloc_count, _, _ = struct.unpack_from(">5I", member)
    if 32 + data_size + reloc_count * 4 > len(member):
        raise ValueError("DAT relocation table is truncated")
    if stride < 4 or stride % 4 != 0:
        raise ValueError("Pointer record stride must be a multiple of four")
    if root + count * stride > data_size:
        raise ValueError("Root pointer table extends beyond DAT data")

    relocations = [
        struct.unpack_from(">I", member, 32 + data_size + i * 4)[0]
        for i in range(reloc_count)
    ]
    if any(slot + 4 > data_size for slot in relocations):
        raise ValueError("DAT has an out-of-range relocation slot")
    targets = {
        struct.unpack_from(">I", member, 32 + slot)[0]
        for slot in relocations
    }
    if any(target >= data_size for target in targets):
        raise ValueError("DAT has an out-of-range relocation target")
    relocation_set = set(relocations)
    targets.update((root, data_size))

    records = []
    for index in range(count):
        slot = root + index * stride
        target = struct.unpack_from(">I", member, 32 + slot)[0]
        if slot not in relocation_set:
            if target != 0:
                raise ValueError(f"Root entry {index} is not a local pointer")
            records.append((index, None, 0, 0))
            continue
        end = min(other for other in targets if other > target)
        nested = sum(target <= pointer < end for pointer in relocations)
        records.append((index, target, end - target, nested))
    return records


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", required=True, type=Path)
    parser.add_argument("--archive", required=True)
    parser.add_argument("--symbol", required=True)
    parser.add_argument("--pointers", required=True, type=int)
    parser.add_argument("--stride", type=int, default=4,
                        help="bytes between pointer fields (default: 4)")
    args = parser.parse_args()
    if args.pointers < 1:
        parser.error("--pointers must be positive")

    member, root = extract_clip(args.iso, args.archive, args.symbol)
    records = inventory(member, root, args.pointers, args.stride)
    print(f"{args.archive}:{args.symbol} root=0x{root:x}")
    print("index  target   span  nested-pointer-slots  first-16-bytes")
    for index, target, span, nested in records:
        if target is None:
            print(f"{index:5}  null")
            continue
        first = member[32 + target:32 + target + min(16, span)]
        print(f"{index:5}  0x{target:05x}  {span:5}  {nested:20}  {first.hex(' ')}")


if __name__ == "__main__":
    main()
