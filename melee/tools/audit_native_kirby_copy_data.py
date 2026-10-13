"""Inventory every retail Kirby copy-data root before native conversion.

Kirby's copy archives do not share one serialized tail layout: after the hat
joint and part descriptor, each fighter stores a different mixture of article,
dynamics, model, animation, visibility and scalar fields.  This audit treats
the retail roots as variable-sized records and verifies the complete archive
family so the native importer cannot silently assume the C declaration's five
pointer tail.
"""

import argparse
from bisect import bisect_right
from collections import Counter
from pathlib import Path

from audit_native_item_scripts import DatView
from extract_native_animation import extract_clip


COPY_ROOTS = (
    ("Mario", "PlKbCpMr.dat", "ftDataKirbyCopyMario"),
    ("Fox", "PlKbCpFx.dat", "ftDataKirbyCopyFox"),
    ("Captain", "PlKbCpCa.dat", "ftDataKirbyCopyCaptain"),
    ("Donkey", "PlKbCpDk.dat", "ftDataKirbyCopyDonkey"),
    ("Koopa", "PlKbCpKp.dat", "ftDataKirbyCopyKoopa"),
    ("Link", "PlKbCpLk.dat", "ftDataKirbyCopyLink"),
    ("Sheik", "PlKbCpSk.dat", "ftDataKirbyCopySeak"),
    ("Ness", "PlKbCpNs.dat", "ftDataKirbyCopyNess"),
    ("Peach", "PlKbCpPe.dat", "ftDataKirbyCopyPeach"),
    ("Popo", "PlKbCpPp.dat", "ftDataKirbyCopyPopo"),
    ("Pikachu", "PlKbCpPk.dat", "ftDataKirbyCopyPikachu"),
    ("Samus", "PlKbCpSs.dat", "ftDataKirbyCopySamus"),
    ("Yoshi", "PlKbCpYs.dat", "ftDataKirbyCopyYoshi"),
    ("Purin", "PlKbCpPr.dat", "ftDataKirbyCopyPurin"),
    ("Mewtwo", "PlKbCpMt.dat", "ftDataKirbyCopyMewtwo"),
    ("Luigi", "PlKbCpLg.dat", "ftDataKirbyCopyLuigi"),
    ("Marth", "PlKbCpMs.dat", "ftDataKirbyCopyMars"),
    ("Zelda", "PlKbCpZd.dat", "ftDataKirbyCopyZelda"),
    ("YoungLink", "PlKbCpCl.dat", "ftDataKirbyCopyClink"),
    ("DrMario", "PlKbCpDr.dat", "ftDataKirbyCopyDrmario"),
    ("Falco", "PlKbCpFc.dat", "ftDataKirbyCopyFalco"),
    ("Pichu", "PlKbCpPc.dat", "ftDataKirbyCopyPichu"),
    ("GameWatch", "PlKbCpGw.dat", "ftDataKirbyCopyGamewatch"),
    ("Ganondorf", "PlKbCpGn.dat", "ftDataKirbyCopyGanon"),
    ("Roy", "PlKbCpFe.dat", "ftDataKirbyCopyEmblem"),
)


def next_target(targets: list[int], source: int) -> int:
    index = bisect_right(targets, source)
    if index == len(targets):
        raise ValueError(f"No object follows Kirby copy root 0x{source:x}")
    return targets[index]


def reference_kind(dat: DatView, slot: int) -> str:
    if slot in dat.relocations:
        return "local"
    if slot in dat.external_slots:
        return "external"
    return "scalar"


def audit(iso: Path) -> None:
    totals = Counter()
    sizes = Counter()
    signatures = Counter()
    for fighter, archive, label in COPY_ROOTS:
        member, extracted_root = extract_clip(iso, archive, label)
        dat = DatView(member)
        root = dat.roots.get(label)
        if root != extracted_root:
            raise ValueError(f"{archive}: root lookup disagrees for {label}")
        targets = sorted({0, dat.data_size, *dat.roots.values(),
                          *(dat.word(slot) for slot in dat.relocations)})
        size = next_target(targets, root) - root
        if size < 4 or size & 3:
            raise ValueError(f"{archive}: invalid root size {size}")
        kinds = []
        article_count = 0
        for slot in range(root, root + size, 4):
            kind = reference_kind(dat, slot)
            kinds.append(kind[0])
            totals[kind] += 1
            if kind == "local" and dat.article(dat.word(slot)):
                article_count += 1
        sizes[size] += 1
        signatures[(size, "".join(kinds))] += 1
        totals["archives"] += 1
        totals["articles"] += article_count
        print(f"{fighter:9} {archive}: {size:2} bytes, {len(kinds):2} words "
              f"[{''.join(kinds)}], {article_count} article candidates")

    observed = (totals["archives"], totals["local"], totals["external"],
                totals["scalar"], totals["articles"], len(signatures),
                tuple(sorted(sizes.items())))
    expected = (25, 103, 0, 127, 42, 22,
                ((20, 2), (24, 2), (28, 1), (32, 5), (36, 4),
                 (40, 3), (44, 4), (48, 2), (52, 1), (56, 1)))
    if observed != expected:
        raise ValueError(f"Kirby copy-data inventory {observed} != {expected}")
    print("root sizes: " + ", ".join(
        f"{size}x{count}" for size, count in sorted(sizes.items())))
    print(f"TOTAL: {totals['archives']} archives, {totals['local']} local tail "
          f"references, {totals['external']} external tail references, "
          f"{totals['scalar']} scalar tail words, "
          f"{totals['articles']} structural article candidates, "
          f"{len(signatures)} root signatures")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", required=True, type=Path)
    args = parser.parse_args()
    audit(args.iso)


if __name__ == "__main__":
    main()
