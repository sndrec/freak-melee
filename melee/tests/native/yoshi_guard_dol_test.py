"""Verify the guard scalar contract against a legally extracted v1.02 DOL.

No disassembler dependency is needed: the relevant PPC D-form word loads and
stores are decoded directly. This test does not extract or distribute assets.
"""
import argparse
import struct
from pathlib import Path


# address, instruction, register, base register, fighter byte offset
GUARD_WORD_ACCESSES = (
    (0x8012BF40, "stw", 0, 30, 0x2364),  # Yoshi shield entry x24 = 0
    (0x8012BF44, "stw", 0, 30, 0x2360),  # Yoshi shield entry x20 = 0
    (0x8012C9DC, "stw", 0, 30, 0x2364),  # Yoshi reflect entry x24 = 0
    (0x8012C9E0, "stw", 0, 30, 0x2360),  # Yoshi reflect entry x20 = 0
    (0x80094140, "stw", 0, 3, 0x235C),   # common powershield input lock
    (0x80093924, "stw", 6, 31, 0x235C),  # common reflect clears input lock
    (0x80095184, "lwz", 0, 4, 0x2360),   # common item throw reads x20
    (0x800951A8, "lwz", 3, 4, 0x2360),
    (0x800951B8, "stw", 0, 4, 0x2360),
    (0x800D8BB8, "lwz", 0, 4, 0x2364),   # common dash catch reads x24
    (0x800D8BD4, "lwz", 3, 4, 0x2364),
    (0x800D8BE4, "stw", 0, 4, 0x2364),
)


def check(data: bytes) -> int:
    if len(data) < 0x100:
        raise ValueError("DOL header is truncated")
    header = struct.unpack_from(">54I", data)
    sections = tuple(zip(header[:18], header[18:36], header[36:54]))
    for address, mnemonic, register, base, offset in GUARD_WORD_ACCESSES:
        location = next((file_offset + address - start
                         for file_offset, start, length in sections
                         if start <= address and address + 4 <= start + length), None)
        if location is None or location + 4 > len(data):
            raise ValueError(f"DOL does not contain instruction {address:08X}")
        word, = struct.unpack_from(">I", data, location)
        expected = ((36 if mnemonic == "stw" else 32) << 26 |
                    register << 21 | base << 16 | offset)
        if word != expected:
            raise ValueError(f"Guard instruction {address:08X}: {word:08X}, "
                             f"expected {expected:08X} ({mnemonic} word slot)")
    return len(GUARD_WORD_ACCESSES)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dol", type=Path, nargs="?")
    parser.add_argument("--iso", type=Path)
    arguments = parser.parse_args()
    if bool(arguments.dol) == bool(arguments.iso):
        parser.error("Supply either an extracted DOL or --iso")
    if arguments.iso:
        with arguments.iso.open("rb") as source:
            header = source.read(0x440)
            if len(header) != 0x440 or header[:6] != b"GALE01" or header[7] != 2:
                raise ValueError("Expected USA v1.02 Melee ISO")
            dol_offset, = struct.unpack_from(">I", header, 0x420)
            source.seek(dol_offset)
            dol_header = source.read(0x100)
            if len(dol_header) != 0x100:
                raise ValueError("ISO contains truncated DOL header")
            words = struct.unpack_from(">54I", dol_header)
            size = max(offset + length for offset, length in zip(words[:18], words[36:54]))
            source.seek(dol_offset)
            data = source.read(size)
            if len(data) != size:
                raise ValueError("ISO contains truncated DOL")
    else:
        data = arguments.dol.read_bytes()
    print(f"Yoshi/common guard DOL scalar contract: {check(data)} instructions passed")
