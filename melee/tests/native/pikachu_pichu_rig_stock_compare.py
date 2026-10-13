"""Decode retail PlCo rig metadata and compare explicit Pikachu/Pichu bindings.

The optional snippets directory is scratch output, never production source.
All four stock costume graphs per fighter must agree with the common mapping's
physical/runtime counts. This is asset-layout evidence, not rendered parity.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
from extract_native_animation import extract_clip


def word(blob, offset):
    return struct.unpack_from(">I", blob, 32 + offset)[0]


def roles():
    source = (ROOT / "src/melee/ft/forward.h").read_text()
    enum = source.split("typedef enum Fighter_Part {", 1)[1].split("} Fighter_Part;", 1)[0]
    enum = re.sub(r"/\*.*?\*/|//[^\n]*", "", enum, flags=re.S)
    names = {}
    value = 0
    for entry in enum.split(","):
        entry = entry.strip()
        if not entry:
            continue
        name, *assigned = entry.split("=")
        if assigned:
            value = int(assigned[0], 0)
        if value < 54:
            names[value] = name.strip()
        value += 1
    names.setdefault(53, "53")
    return names


def read_rigs(iso):
    names = roles()
    common, root = extract_clip(iso, "PlCo.dat", "ftLoadCommonData")
    table, omissions_table = word(common, root + 16), word(common, root + 20)
    result = {"iso": str(iso), "common_sha256": hashlib.sha256(common).hexdigest(), "fighters": {}}
    for family, kind, stem, suffix, colors, expected in (
        ("pikachu", 12, "Pikachu", "Pk", ("Nr", "Re", "Bu", "Gr"), 53),
        ("pichu", 23, "Pichu", "Pc", ("Nr", "Re", "Bu", "Gr"), 46),
    ):
        descriptor = word(common, table + 4 * kind)
        inverse, semantic, count = (word(common, descriptor + i * 4) for i in range(3))
        mapping = list(common[32 + semantic:32 + semantic + 54])
        pairs = [{"part": i, "role": names[i], "joint": joint}
                 for i, joint in enumerate(mapping) if joint != 255]
        inverse_mapping = list(common[32 + inverse:32 + inverse + count])
        assert all(inverse_mapping[pair["joint"]] == pair["part"] for pair in pairs)
        reconstructed_inverse = [255] * count
        for pair in pairs:
            reconstructed_inverse[pair["joint"]] = pair["part"]
        assert reconstructed_inverse == inverse_mapping
        omit_descriptor = word(common, omissions_table + kind * 4)
        omissions = []
        if omit_descriptor:
            records, omit_count = word(common, omit_descriptor), word(common, omit_descriptor + 4)
            omissions = [list(common[32 + records + i * 4:32 + records + (i + 1) * 4])
                         for i in range(omit_count)]
        costumes = []
        for color in colors:
            archive = "Pl" + suffix + color + ".dat"
            symbol = "Ply" + stem + "5K" + (color if color != "Nr" else "") + "_Share_joint"
            costume, joint_root = extract_clip(iso, archive, symbol)
            seen = set()

            def visit(offset):
                while offset:
                    assert offset not in seen, "Joint graph cycle/alias"
                    seen.add(offset)
                    # HSD preorder traversal does not enter instance children.
                    if not word(costume, offset + 4) & (1 << 12):
                        visit(word(costume, offset + 8))
                    offset = word(costume, offset + 12)

            visit(joint_root)
            assert len(seen) == expected
            assert count == len(seen) + len(omissions)
            assert all(pair["joint"] < count for pair in pairs)
            costumes.append({"archive": archive, "root": symbol, "physical_joints": len(seen),
                             "sha256": hashlib.sha256(costume).hexdigest()})
        result["fighters"][family] = {"internal_kind": kind, "runtime_parts": count,
            "bones": pairs, "part_to_joint": mapping,
            "joint_to_part": inverse_mapping,
            "omissions": omissions, "costumes": costumes,
            "raw_part5_roles": [pair["role"] for pair in pairs if pair["joint"] == 5]}
    return result


def validate_clone(family, rig):
    stem = "Pikachu" if family == "pikachu" else "Pichu"
    source = (ROOT / f"src/melee/ft/kinds/ft{stem}/ft{family}_clone.c").read_text()
    match = re.search(r"NativeFighterBone\s+ft" + stem + r"Clone_Bones\[\]\s*=\s*\{(.*?)\};", source, re.S)
    assert match, family + " missing explicit bindings"
    pairs = re.findall(r"\{\s*(FtPart_\w+|53)\s*,\s*(\d+)\s*\}", match[1])
    assert pairs == [(pair["role"], str(pair["joint"])) for pair in rig["bones"]]
    assert re.search(r"\.bones\s*=\s*ft" + stem + r"Clone_Bones\b", source)
    assert re.search(r"\.bone_count\s*=\s*sizeof\(ft" + stem + r"Clone_Bones\)", source)
    assert int(re.search(r"\.joint_count\s*=\s*(\d+)", source)[1]) == rig["costumes"][0]["physical_joints"]
    omissions = re.search(r"NativeFighterPartOmission\s+ft" + stem + r"Clone_Omissions\[\]\s*=\s*\{(.*?)\};", source, re.S)
    if rig["omissions"]:
        assert omissions
        rows = [list(map(int, values)) for values in re.findall(
            r"\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}", omissions[1])]
        assert rows == rig["omissions"]
        assert re.search(r"\.omissions\s*=\s*ft" + stem + r"Clone_Omissions\b", source)
        assert re.search(r"\.omission_count\s*=\s*sizeof\(ft" + stem + r"Clone_Omissions\)", source)
    else:
        assert omissions is None
        assert not re.search(r"\.omissions\s*=|\.omission_count\s*=", source)
    return hashlib.sha256(source.encode()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--snippets", type=Path)
    parser.add_argument("--validate-clones", action="store_true")
    args = parser.parse_args()
    result = read_rigs(args.iso.resolve())
    for family, rig in result["fighters"].items():
        if args.validate_clones:
            rig["clone_sha256"] = validate_clone(family, rig)
        if args.snippets:
            args.snippets.mkdir(parents=True, exist_ok=True)
            (args.snippets / (family + "_bones.inc")).write_text("".join(
                f"    {{ {pair['role']}, {pair['joint']} }},\n" for pair in rig["bones"]), newline="\n")
            (args.snippets / (family + "_omissions.inc")).write_text("".join(
                "    { " + ", ".join(map(str, row)) + " },\n" for row in rig["omissions"]), newline="\n")
        print(f"{family}: {len(rig['bones'])} semantic bindings, {rig['runtime_parts']} runtime parts, "
              f"{len(rig['omissions'])} omissions, four costume graphs; raw part5={rig['raw_part5_roles']}")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
