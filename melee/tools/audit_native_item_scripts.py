"""Inventory item-script entries in the retail common and fighter DATs.

The importer recognizes article/state tables by their serialized pointer
structure. This tool independently follows that same structure on the disc and
reports which item scripts need a native bytecode converter. It does not run
any commands or establish gameplay fidelity.
"""

import argparse
from collections import Counter
from pathlib import Path
import struct

from audit_native_action_scripts import disc_fighters
from extract_native_animation import extract_clip


class DatView:
    def __init__(self, member: bytes):
        if len(member) < 32:
            raise ValueError("Truncated DAT header")
        size, self.data_size, reloc_count, root_count, extern_count = (
            struct.unpack_from(">5I", member)
        )
        if size > len(member):
            raise ValueError("Truncated DAT")
        self.member = member
        table = 32 + self.data_size
        self.relocations = {
            struct.unpack_from(">I", member, table + index * 4)[0]
            for index in range(reloc_count)
        }
        roots = table + reloc_count * 4
        externs = roots + root_count * 8
        strings = externs + extern_count * 8
        if strings > size:
            raise ValueError("Truncated DAT tables")
        self.roots = {}
        for index in range(root_count):
            offset, label = struct.unpack_from(">II", member,
                                               roots + index * 8)
            label_start = strings + label
            if label_start >= size:
                raise ValueError("DAT root label out of range")
            end = member.index(0, label_start, size)
            self.roots[member[label_start:end].decode("ascii")] = offset
        self.external_slots = set()
        for index in range(extern_count):
            link = struct.unpack_from(">I", member, externs + index * 8)[0]
            visited = set()
            while link != 0xFFFFFFFF:
                if link in visited:
                    raise ValueError("Cyclic DAT external chain")
                visited.add(link)
                self.external_slots.add(link)
                link = self.word(link)

    def word(self, offset: int) -> int:
        if offset < 0 or offset + 4 > self.data_size:
            raise ValueError(f"DAT word 0x{offset:x} out of range")
        return struct.unpack_from(">I", self.member, 32 + offset)[0]

    def reference(self, slot: int) -> tuple[str, int]:
        if slot & 3 or slot + 4 > self.data_size:
            return "invalid", 0
        value = self.word(slot)
        if slot in self.relocations:
            return "local", value
        if slot in self.external_slots:
            return "external", value
        return ("null" if value == 0 else "invalid"), value

    def article(self, source: int, allow_null_attributes: bool = False) -> bool:
        if source < 0 or source + 24 > self.data_size:
            return False
        fields = [self.reference(source + index * 4) for index in range(6)]
        kinds = [kind for kind, _ in fields]
        if "invalid" in kinds or (
            kinds[0] != "local" and
            not (allow_null_attributes and kinds[0] == "null")
        ):
            return False
        if kinds[3] == "local" and any(
            self.reference(fields[3][1] + index * 4)[0] == "invalid"
            for index in range(4)
        ):
            return False
        if kinds[4] == "local":
            model = fields[4][1]
            if self.reference(model)[0] == "invalid" or any(
                self.reference(model + index * 4)[0] in ("local", "external")
                for index in range(1, 4)
            ):
                return False
        if kinds[5] == "local":
            dynamics = fields[5][1]
            count = self.word(dynamics)
            desc_kind = self.reference(dynamics + 4)[0]
            if count > 32 or (count == 0 and desc_kind != "null") or (
                count != 0 and desc_kind != "local"
            ):
                return False
        return True

    def state_scripts(self, article: int) -> tuple[int, list[tuple[str, int]]]:
        kind, source = self.reference(article + 12)
        if kind == "null":
            return 0, []
        if kind != "local":
            raise ValueError(f"Item state table at 0x{article:x} is {kind}")
        scripts = []
        for index in range(256):
            at = source + index * 16
            if index and self.article(at):
                return index, scripts
            if any(self.reference(at + field * 4)[0] == "invalid"
                   for field in range(4)):
                if index == 0:
                    raise ValueError(f"Empty item state table 0x{source:x}")
                return index, scripts
            kind, target = self.reference(at + 12)
            if kind != "null":
                scripts.append((kind, target))
        raise ValueError(f"Unterminated item state table 0x{source:x}")


def common_articles(dat: DatView):
    root = dat.roots["itPublicData"]
    # It_PKind_Start (0xA1) - It_Kind_Kuriboh (43) = 118.
    for slot, count in ((root + 4, 43), (root + 8, 118), (root + 12, 47)):
        kind, table = dat.reference(slot)
        if kind != "local":
            raise ValueError(f"Missing common item table at 0x{slot:x}")
        for index in range(count):
            ref, article = dat.reference(table + index * 4)
            if ref == "null":
                continue
            if ref != "local" or not dat.article(article,
                                                   slot == root + 8 and
                                                   index == 117):
                raise ValueError(f"Invalid item article at 0x{table + index * 4:x}")
            yield article


def fighter_articles(dat: DatView):
    # This is deliberately structural: some non-article fighter slots resemble
    # article descriptors, but none adds a script in the retail corpus. The
    # native importer uses fighter_article_mask for its actual article count.
    roots = [offset for label, offset in dat.roots.items()
             if label.startswith("ftData")]
    if len(roots) != 1:
        raise ValueError("Expected one fighter data root")
    kind, table = dat.reference(roots[0] + 72)
    if kind == "null":
        return
    if kind != "local":
        raise ValueError("Invalid fighter article table")
    for index in range(16):
        ref, article = dat.reference(table + index * 4)
        if ref == "local" and dat.article(article):
            yield article


def item_width(opcode: int, raw: int) -> int:
    if opcode < 10:
        return 2 if opcode in (5, 7) else 1
    if opcode == 10:
        return 5
    if opcode == 11:
        return 6
    if opcode == 16:
        subopcode = (raw >> 18) & 0xFF
        return 3 if subopcode in (0, 1, 2, 10, 11) else 2
    return 1 if opcode <= 25 else 0


def walk_scripts(dat: DatView, entries: list[int]) -> tuple[int, Counter, Counter]:
    visited = set()
    payload = set()
    pending = entries[:]
    counts = Counter()
    subopcodes = Counter()
    while pending:
        at = pending.pop()
        while at not in visited:
            if at in payload:
                raise ValueError(f"Item script jumps into payload at 0x{at:x}")
            raw = dat.word(at)
            opcode = raw >> 26
            width = item_width(opcode, raw)
            if width == 0 or at + width * 4 > dat.data_size:
                raise ValueError(f"Invalid item opcode {opcode} at 0x{at:x}")
            visited.add(at)
            for word in range(1, width):
                word_at = at + word * 4
                if word_at in visited or word_at in payload:
                    raise ValueError(f"Item payload overlaps at 0x{word_at:x}")
                payload.add(word_at)
            counts[opcode] += 1
            if opcode == 16:
                subopcodes[(raw >> 18) & 0xFF] += 1
            if opcode in (5, 7):
                kind, target = dat.reference(at + 4)
                if kind == "local":
                    pending.append(target)
                elif kind != "null":
                    raise ValueError(f"Invalid item branch at 0x{at:x}")
            if opcode in (0, 6, 7):
                break
            at += width * 4
    return len(visited), counts, subopcodes


def audit_bank(name: str, member: bytes, articles) -> tuple[int, int, Counter, int, Counter, Counter]:
    dat = DatView(member)
    article_count = 0
    state_count = 0
    scripts = Counter()
    entries = []
    for article in articles(dat):
        count, refs = dat.state_scripts(article)
        article_count += 1
        state_count += count
        for kind, target in refs:
            scripts[kind] += 1
            if kind == "local":
                entries.append(target)
                opcode = dat.word(target) >> 26
                scripts[f"opcode_{opcode}"] += 1
    instruction_count, opcodes, subopcodes = walk_scripts(dat, entries)
    article_label = "articles" if name == "ItCo.usd" else "candidate articles"
    print(f"{name}: {article_count} {article_label}, {state_count} states, "
          f"{scripts['local']} local scripts, "
          f"{scripts['external']} external scripts, "
          f"{instruction_count} instruction starts")
    return (article_count, state_count, scripts, instruction_count,
            opcodes, subopcodes)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", required=True, type=Path)
    args = parser.parse_args()
    member, _ = extract_clip(args.iso, "ItCo.usd", "itPublicData")
    common = audit_bank("ItCo.usd", member, common_articles)
    fighter_count = states = 0
    fighters = Counter()
    instructions = common[3]
    opcodes = Counter(common[4])
    subopcodes = Counter(common[5])
    for name, fighter in disc_fighters(args.iso):
        (articles, bank_states, scripts, bank_instructions,
         bank_opcodes, bank_subopcodes) = audit_bank(name, fighter,
                                                     fighter_articles)
        fighter_count += articles
        states += bank_states
        fighters.update(scripts)
        instructions += bank_instructions
        opcodes.update(bank_opcodes)
        subopcodes.update(bank_subopcodes)
    if (common[0], common[1], common[2]["local"], fighters["local"],
        instructions) != (
        98, 226, 152, 116, 1046
    ):
        raise ValueError("Item script inventory differs from the retail corpus")
    print(f"TOTAL: common {common[0]} articles / {common[1]} states / "
          f"{common[2]['local']} local / {common[2]['external']} external; "
          f"fighter {fighter_count} candidate articles / {states} states / "
          f"{fighters['local']} local / {fighters['external']} external; "
          f"{instructions} distinct instruction starts")
    print("ENTRY OPCODES:", " ".join(
        f"{opcode}:{count}"
        for opcode, count in sorted((int(key[7:]), value) for key, value in
                                    (common[2] + fighters).items()
                                    if key.startswith("opcode_"))
    ))
    print("OPCODES:", " ".join(f"{opcode}:{count}"
                                 for opcode, count in sorted(opcodes.items())))
    print("OPCODE 16 SUBOPCODES:", " ".join(
        f"{subopcode}:{count}" for subopcode, count in
        sorted(subopcodes.items())))


if __name__ == "__main__":
    main()
