"""Verify Samus's owned rig facts against retail v1.02 common/costume data.

This checks archive layout and authoring bindings, not rendered or gameplay parity.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re

from fox_falco_rig_stock_compare import ROOT, extract_clip, roles, word


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    common, root = extract_clip(args.iso.resolve(), "PlCo.dat", "ftLoadCommonData")
    table, omissions_table = word(common, root + 16), word(common, root + 20)
    descriptor = word(common, table + 4 * 13)
    inverse, semantic, count = (word(common, descriptor + i * 4) for i in range(3))
    names = roles()
    mapping = list(common[32 + semantic:32 + semantic + 54])
    pairs = [(names[part], joint) for part, joint in enumerate(mapping) if joint != 255]
    assert count == 60 and len(pairs) == 41
    reconstructed_inverse = [255] * count
    for part, joint in enumerate(mapping):
        if joint != 255:
            assert joint < count
            reconstructed_inverse[joint] = part
    assert reconstructed_inverse == list(common[32 + inverse:32 + inverse + count])
    omit_descriptor = word(common, omissions_table + 13 * 4)
    assert not omit_descriptor or word(common, omit_descriptor + 4) == 0

    source_path = ROOT / "src/melee/ft/kinds/ftSamus/ftsamus_clone.c"
    source = source_path.read_text(encoding="utf-8")
    bindings = re.search(r"NativeFighterBone\s+ftSamusClone_Bones\[\]\s*=\s*\{(.*?)\};", source, re.S)
    assert bindings, "Samus authoring definition has no explicit semantic bindings"
    authored = re.findall(r"\{\s*(FtPart_\w+|53)\s*,\s*(\d+)\s*\}", bindings[1])
    assert authored == [(role, str(joint)) for role, joint in pairs]
    assert re.search(r"\.bones\s*=\s*ftSamusClone_Bones\b", source)
    assert re.search(r"\.bone_count\s*=\s*sizeof\(ftSamusClone_Bones\)", source)
    assert int(re.search(r"\.joint_count\s*=\s*(\d+)", source)[1]) == count
    costumes = []
    for color in ("Bk", "Gr", "La", "Nr", "Pi"):
        archive = "PlSs" + color + ".dat"
        symbol = "PlySamus5K" + (color if color != "Nr" else "") + "_Share_joint"
        costume, joint_root = extract_clip(args.iso.resolve(), archive, symbol)
        seen = set()

        def visit(offset):
            while offset:
                assert offset not in seen, "Cyclic costume joint graph"
                seen.add(offset)
                visit(word(costume, offset + 8))
                offset = word(costume, offset + 12)

        visit(joint_root)
        assert len(seen) == count, archive
        costumes.append({"archive": archive, "root": symbol, "physical_joints": len(seen),
                         "sha256": hashlib.sha256(costume).hexdigest()})
    receipt = {"passed": True, "scope": "retail rig facts and owned authoring bindings",
               "internal_kind": 13, "runtime_parts": count, "semantic_bindings": len(pairs),
               "part_to_joint": mapping, "omissions": [], "costumes": costumes,
               "common_sha256": hashlib.sha256(common).hexdigest(),
               "clone_sha256": hashlib.sha256(source_path.read_bytes()).hexdigest()}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print("Samus: 41 exact semantic bindings, 60 joints, no omissions, all five costumes PASS")


if __name__ == "__main__":
    main()
