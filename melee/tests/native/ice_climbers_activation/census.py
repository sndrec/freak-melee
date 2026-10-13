"""Read-only stock source/retail asset census; no module compilation or gameplay."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import sys


def clean(text):
    return re.sub(r"/\*.*?\*/|//[^\n]*", "", text, flags=re.S)


def entries(source, start):
    depth, begin, output = 0, start + 1, []
    for pos in range(start + 1, len(source)):
        char = source[pos]
        if char in "{([":
            depth += 1
        elif char in ")]}" and depth:
            depth -= 1
        elif char == "}" and not depth:
            if source[begin:pos].strip():
                output.append(source[begin:pos].strip())
            return output
        elif char == "," and not depth:
            output.append(source[begin:pos].strip())
            begin = pos + 1
    raise ValueError("Unterminated initializer")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root = args.root.resolve()
    sys.path.insert(0, str(root / "tools"))
    from extract_native_animation import extract_clip
    word = lambda blob, off: struct.unpack_from(">I", blob, 32 + off)[0]
    files = sorted(set(list((root / "src/melee/ft/kinds/ftPopo").glob("*.[ch]")) +
                       list((root / "src/melee/ft/kinds/ftNana").glob("*.[ch]")) +
                       list((root / "src/melee/it/kinds").glob("itclimbers*.[ch]"))))
    extra = ["src/melee/pl/player.c", "src/melee/ft/types.h", "src/melee/ft/ftdata.c",
             "src/melee/ft/ftmovement.c", "src/melee/ft/ftcommon.c", "src/melee/ft/ft_0877.c",
             "src/melee/ft/ftcpuattack.c", "src/melee/ft/fighter.c", "src/melee/ft/ftcolanim.c",
             "src/melee/ft/kinds/ftCommon/ftCo_0A01.c", "src/melee/ft/kinds/ftCommon/ftCo_Landing.c",
             "src/melee/ft/kinds/ftCommon/ftpickupitem.c", "src/melee/ft/ftcharacter_assets.c",
             "src/melee/ft/ftcharacter_asset_layout.h", "src/melee/ft/native_fighter_runtime.c",
             "src/melee/ft/native_fighter_runtime.h", "src/melee/ft/native_fighter_article_bindings.c",
             "src/melee/it/itCharItems.h", "src/melee/it/it_3F2F.c",
             "src/melee/ft/native_fighter_target.h", "src/melee/ft/ftanim.c",
             "src/melee/ef/efsync.c", "src/melee/ef/eflib.c",
             "tools/native_fighter_script_cpu.c", "tools/native_fighter_script_primitives.c",
             "tools/extract_native_animation.py", "tools/prepare_native_assets.py",
             "src/melee/ft/forward.h"]
    files += [root / path for path in extra]
    result = {"scope": "read-only census, not implementation or parity acceptance", "sources": {},
              "motion_tables": {}, "kind_tables": {}, "retail": {"iso": str(args.iso.resolve())}}
    for path in files:
        result["sources"][path.relative_to(root).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
    for family, table in (("ftPopo", "ftPp_Init_MotionStateTable"), ("ftNana", "ftNn_Init_MotionStateTable")):
        path = root / f"src/melee/ft/kinds/{family}/{family.lower()}.c"
        source = clean(path.read_text(encoding="utf-8"))
        start = source.index("{", source.index(table))
        rows = []
        for index, row in enumerate(entries(source, start)):
            fields = entries(row, row.index("{"))
            assert len(fields) == 8
            rows.append({"id": 341 + index, "animation": fields[0], "flags": fields[1:3],
                         "phases": dict(zip(("animation", "input", "physics", "collision"), fields[3:7])),
                         "camera": fields[7]})
        result["motion_tables"][family] = rows
    source = clean((root / "src/melee/ft/ftdata.c").read_text(encoding="utf-8"))
    for match in re.finditer(r"\b(\w+)\[Ft_Kind_RuntimeCapacity\]\s*=\s*\{", source):
        rows = entries(source, match.end() - 1)
        if len(rows) > 11:
            result["kind_tables"][match[1]] = {"Popo": rows[10], "Nana": rows[11]}
    enum = clean((root / "src/melee/ft/forward.h").read_text(encoding="utf-8").split(
        "typedef enum Fighter_Part {", 1)[1].split("} Fighter_Part;", 1)[0])
    roles, index = {}, 0
    for token in enum.split(","):
        if not token.strip():
            continue
        name, *assigned = token.strip().split("=")
        if assigned:
            index = int(assigned[0], 0)
        roles[index] = name.strip()
        index += 1
    common, common_root = extract_clip(args.iso, "PlCo.dat", "ftLoadCommonData")
    result["retail"]["PlCo_sha256"] = hashlib.sha256(common).hexdigest()
    table, omissions = word(common, common_root + 16), word(common, common_root + 20)
    for family, kind, suffix, stem, colors in (("Popo", 10, "Pp", "Popo", ("Nr", "Gr", "Or", "Re")),
                                             ("Nana", 11, "Nn", "Nana", ("Nr", "Ye", "Aq", "Wh"))):
        desc = word(common, table + 4 * kind)
        inverse, semantic, count = [word(common, desc + 4 * i) for i in range(3)]
        mapping = list(common[32 + semantic:32 + semantic + 54])
        backward = list(common[32 + inverse:32 + inverse + count])
        pairs = [{"part": i, "role": roles.get(i, str(i)), "joint": value}
                 for i, value in enumerate(mapping) if value != 255]
        rebuilt = [255] * count
        for pair in pairs:
            rebuilt[pair["joint"]] = pair["part"]
        assert rebuilt == backward
        omit_desc = word(common, omissions + 4 * kind)
        omitted = []
        if omit_desc:
            data, n = word(common, omit_desc), word(common, omit_desc + 4)
            omitted = [list(common[32 + data + 4 * i:32 + data + 4 * (i + 1)]) for i in range(n)]
        costumes = []
        for color in colors:
            name = f"Pl{suffix}{color}.dat"
            symbol = f"Ply{stem}5K{'' if color == 'Nr' else color}_Share_joint"
            archive, costume_root = extract_clip(args.iso, name, symbol)
            visited = set()
            def visit(offset):
                while offset:
                    assert offset not in visited
                    visited.add(offset)
                    if not word(archive, offset + 4) & (1 << 12):
                        visit(word(archive, offset + 8))
                    offset = word(archive, offset + 12)
            visit(costume_root)
            assert len(visited) + len(omitted) == count
            costumes.append({"archive": name, "root": symbol, "physical_joints": len(visited),
                             "sha256": hashlib.sha256(archive).hexdigest()})
        dat, data_root = extract_clip(args.iso, f"Pl{suffix}.dat", f"ftData{family}")
        attrs = word(dat, data_root + 4)
        item_table = word(dat, data_root + 0x48)
        article_offsets = [word(dat, item_table + i * 4) for i in range(3)]
        string_attributes = word(dat, article_offsets[2] + 4)
        result["retail"][family] = {"internal_kind": kind, "runtime_parts": count,
            "bones": pairs, "part_to_joint": mapping, "joint_to_part": backward,
            "omissions": omitted, "costumes": costumes, "data_sha256": hashlib.sha256(dat).hexdigest(),
            "special_attributes_offset": attrs,
            "special_attributes_0x15C_sha256": hashlib.sha256(dat[32 + attrs:32 + attrs + 0x15C]).hexdigest(),
            "article_table_offset": item_table, "article_offsets": article_offsets,
            "string_attribute_offset": string_attributes,
            "string_link_count": word(dat, string_attributes),
            "string_joint_roots": [word(dat, string_attributes + off) for off in (0x24, 0x28)]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    for family in ("Popo", "Nana"):
        rig = result["retail"][family]
        print(f"{family}: {len(rig['bones'])} roles, {rig['runtime_parts']} parts, "
              f"{len(rig['omissions'])} omissions, four costumes; articles={rig['article_offsets']}")


if __name__ == "__main__":
    main()
