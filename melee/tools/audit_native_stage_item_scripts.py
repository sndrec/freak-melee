"""Audit stage-owned item articles and ALDYakuAll scripts on the retail disc.

This follows the two stage roots read by Ground_801C3828. It validates source
item command graphs but does not import stage data or execute a stage.
"""

import argparse
from collections import Counter
from pathlib import Path
import struct

from audit_native_item_scripts import DatView, walk_scripts
from extract_native_animation import extract_clip
from prepare_native_assets import read_exact


def disc_stage_names(iso: Path):
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
            name, _, _ = struct.unpack_from(">III", fst, index * 12)
            if name >> 24:
                continue
            start = name & 0xFFFFFF
            filename = names[start:names.index(0, start)].decode("ascii")
            if filename.startswith("Gr") and filename.endswith(".dat"):
                yield filename


def local_list(dat: DatView, root: int, first: int):
    if first == 1 and dat.reference(root)[0] != "null":
        raise ValueError("ALDYakuAll slot zero is not empty")
    for index in range(first, 64):
        kind, target = dat.reference(root + index * 4)
        if kind == "null":
            return
        if kind != "local":
            raise ValueError(f"Invalid stage list entry at 0x{root + index * 4:x}")
        yield target
    raise ValueError(f"Unterminated stage list at 0x{root:x}")


def audit_stage(name: str, dat: DatView):
    item_root = dat.roots["itemdata"]
    ald_root = dat.roots["ALDYakuAll"]
    articles = 0
    state_scripts = []
    for record in local_list(dat, item_root, 0):
        kind, article = dat.reference(record + 4)
        if kind != "local" or not dat.article(article):
            raise ValueError(f"Invalid stage item record at 0x{record:x}")
        articles += 1
        for script_kind, target in dat.state_scripts(article)[1]:
            if script_kind != "local":
                raise ValueError(f"Invalid stage item script in 0x{article:x}")
            state_scripts.append(target)
    ald_scripts = list(local_list(dat, ald_root, 1))
    instructions, opcodes, subopcodes = walk_scripts(
        dat, state_scripts + ald_scripts
    )
    return articles, len(state_scripts), len(ald_scripts), instructions, opcodes, subopcodes


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", required=True, type=Path)
    args = parser.parse_args()
    totals = Counter()
    opcodes = Counter()
    subopcodes = Counter()
    for name in disc_stage_names(args.iso):
        member, _ = extract_clip(args.iso, name, "map_head")
        stage = DatView(member)
        (articles, item_scripts, ald_scripts, instructions,
         stage_opcodes, stage_subopcodes) = audit_stage(name, stage)
        totals.update(archives=1, articles=articles, item_scripts=item_scripts,
                      ald_scripts=ald_scripts, instructions=instructions)
        opcodes.update(stage_opcodes)
        subopcodes.update(stage_subopcodes)
        if articles or item_scripts or ald_scripts != 1:
            print(f"{name}: {articles} item articles, {item_scripts} item-state "
                  f"scripts, {ald_scripts} ALDYakuAll scripts, "
                  f"{instructions} instruction starts")
    expected = (71, 53, 62, 82, 269)
    observed = tuple(totals[key] for key in
                     ("archives", "articles", "item_scripts",
                      "ald_scripts", "instructions"))
    if observed != expected:
        raise ValueError(f"Stage item inventory {observed} != {expected}")
    print(f"TOTAL: {totals['archives']} archives, {totals['articles']} item "
          f"articles, {totals['item_scripts']} item-state scripts, "
          f"{totals['ald_scripts']} ALDYakuAll scripts, "
          f"{totals['instructions']} distinct instruction starts")
    print("OPCODES:", " ".join(f"{code}:{count}" for code, count in
                                sorted(opcodes.items())))
    print("OPCODE 16 SUBOPCODES:", " ".join(
        f"{code}:{count}" for code, count in sorted(subopcodes.items())))


if __name__ == "__main__":
    main()
