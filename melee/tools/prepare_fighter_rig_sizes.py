"""Extract stock skeleton sizes and each costume's fighter-model bindings."""
import argparse
import hashlib
from pathlib import Path
import re
import struct

from fighter_animation_io import Dat, disc_file
from prepare_native_assets import extract_dol, extract_symbol, write_if_changed


def costume_rows(dat, fighter_root, joints):
    model = dat.u32(fighter_root + 8)
    groups, visibility, textures, texture_table = struct.unpack_from(">4I", dat.data, model)
    relocations = set(dat.relocs)
    # Use the same table boundary as HSD_DatNextTarget in the fighter loader.
    targets = (dat.u32(slot) for slot in relocations)
    next_target = min((target for target in targets if target > visibility), default=len(dat.data))
    variants, remainder = divmod(next_target - visibility, 16)
    if remainder or not 1 <= variants <= 8 or textures > 5 or groups > 11:
        raise ValueError("Unexpected stock fighter model table")
    rows = []
    for v in range(variants):
        draws = 0
        for group in (0, 1, 3):
            entry = visibility + v * 16 + group * 4
            if entry not in relocations:
                entry = visibility + group * 4
            if entry not in relocations:
                continue
            lookup = dat.u32(entry)
            for i in range(groups):
                count, choices = struct.unpack_from(">2I", dat.data, lookup + i * 8)
                for j in range(count):
                    size, start = struct.unpack_from(">2I", dat.data, choices + j * 8)
                    if size:
                        draws = max(draws, 1 + max(dat.data[start:start + size]))
        entry = texture_table + v * 4
        if entry not in relocations:
            entry = texture_table
        if textures and entry not in relocations:
            raise ValueError("Missing stock texture slot table")
        slots = struct.unpack_from(f">{textures}H", dat.data, dat.u32(entry)) if textures else ()
        values = ", ".join(map(str, slots)) or "0"
        rows.append(f"{{ {joints}, {draws}, {textures}, {{ {values} }} }}")
    return "    { " + ", ".join(rows) + " },\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--costumes-output", type=Path, required=True)
    args = parser.parse_args()
    source = disc_file(args.iso, "PlCo.dat")
    if hashlib.sha256(source).hexdigest() != "63841336337eb5a7366b06ccc60ea4bd37c3604ab56e19939d78b9aa9cdd234c":
        raise ValueError("Expected PlCo.dat from the USA v1.02 Melee disc")
    dat = Dat(source)
    root = dat.roots["ftLoadCommonData"]
    parts = dat.u32(root + 16)
    omissions = dat.u32(root + 20)
    relocations = set(dat.relocs)
    rows = []
    skeletons = []
    # The last row is the common throw skeleton, immediately after Sandbag.
    for kind in range(0x22):
        parts_slot = parts + kind * 4
        omission_slot = omissions + kind * 4
        if parts_slot not in relocations:
            raise ValueError(f"Missing skeleton for fighter kind {kind}")
        count = dat.u32(dat.u32(parts_slot) + 8)
        inserted = dat.u32(dat.u32(omission_slot) + 4) if omission_slot in relocations else 0
        if not (0 < count <= 140 and inserted <= min(count, 32)):
            raise ValueError(f"Invalid stock skeleton for fighter kind {kind}")
        rows.append(f"    {{ {count}, {inserted} }},\n")
        skeletons.append(count - inserted)
    config = Path(__file__).resolve().parents[1] / "config/GALE01"
    digest = re.search(r"^hash: ([0-9a-f]{40})$", (config / "config.yml").read_text(), re.M)[1]
    dol, sections = extract_dol(args.iso, digest)
    table = extract_symbol(dol, sections, (config / "symbols.txt").read_text(), "ftData_803C1F40")

    def name(address):
        start = next(o + address - base for o, base, n in sections if base <= address < base + n)
        return dol[start:dol.index(0, start)].decode("ascii")

    costumes = []
    for kind, (file_ptr, root_ptr) in enumerate(struct.iter_unpack(">II", table)):
        fighter = Dat(disc_file(args.iso, name(file_ptr)))
        costumes.append(costume_rows(fighter, fighter.roots[name(root_ptr)], skeletons[kind]))
    write_if_changed(args.output, "".join(rows).encode("ascii"))
    write_if_changed(args.costumes_output, "".join(costumes).encode("ascii"))


if __name__ == "__main__":
    main()
