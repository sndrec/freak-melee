"""Extract the actual Peach rare constructor and Bomb/Saturn throw bodies.

The original console reference corrects only the two decomp union aliases to
their actual item types: retail stores Bomb scale at item+0xDE8 and Saturn at
item+0xDE0. The legacy native body is retained separately as a negative control.
This generates source includes; it does not build or launch the game.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import struct
import subprocess

ORIGINAL_REF = "4df2ecc99"
SOURCES = {
    "src/melee/it/kinds/inlines.h": ["Item_InitSpawn"],
    "src/melee/it/kinds/itpeachturnip.c": ["it_802BD4AC", "itPeachTurnip_Logic56_Thrown", "itPeachTurnip_Logic56_Dropped"],
    "src/melee/it/kinds/itbombhei.c": ["itBombhei_Logic6_Spawned", "it_3F14_Logic6_Thrown", "it_3F14_Logic6_Dropped", "itBombhei_UnkMotion10_Anim"],
    "src/melee/it/kinds/itdosei.c": ["HSD_JObjSetRotationZero", "itDosei_FacingAngle", "itDosei_GetJObj", "itDosei_Logic7_Spawned", "itDosei_Logic7_Thrown", "itDosei_Logic7_Dropped"],
}
RETAIL = {
    "address": 0x802BD4AC, "size": 0x17C,
    "sha256": "3f500c05cdf4c6735c7abca7c62598aea32265f294dd8d2ad65bb13c0d88be51",
}
STORES = {0x802BD5F4: 0xD0030DE8, 0x802BD600: 0xD0030DE0,
          0x802BD60C: 0xD0030DE0}


def body(source, name):
    match = re.search(r"(?m)^(?:static\s+)?(?:inline\s+)?(?:void|bool|f32|HSD_JObj\*|Item_GObj\*)\s+" +
                      re.escape(name) + r"\([^;{}]*\)\s*\{", source)
    if not match:
        raise ValueError("missing actual production body: " + name)
    depth, end = 1, match.end()
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


def renamed(source, prefix):
    for names in SOURCES.values():
        for name in names:
            source = re.sub(r"\b" + name + r"\b", prefix + name, source)
    return source


def reference_private(source):
    # The reference uses actual historical private layouts, including its pointer
    # widening alias bug. Only representation is projected for semantic comparison;
    # callback order/math/branch bodies are not rewritten into an equivalent.
    return re.sub(r"\b(?:item|ip|temp_r30|temp_r29)->xDD4_itemVar\.", "reference_vars.", source)


def reference_layout(root, receipts):
    result, fields = [], {}
    for path, names in {"src/melee/it/itCommonItems.h": ["itBombHei_ItemVars", "itDosei_ItemVars"],
                        "src/melee/it/itCharItems.h": ["itPeachTurnip_ItemVars"]}.items():
        original = subprocess.check_output(["git", "show", ORIGINAL_REF + ":" + path], cwd=root).decode()
        current = (root / path).read_text()
        receipts[path] = {"original_sha256": hashlib.sha256(original.encode()).hexdigest(),
                          "candidate_sha256": hashlib.sha256(current.encode()).hexdigest()}
        for name in names:
            declaration = re.search(r"typedef struct " + name + r" \{.*?\} " + name + r";", original, re.S).group()
            fields[name] = re.findall(r"\b(x\w+|_E\w+)\s*;", declaration)
            current_declaration = re.search(r"typedef struct " + name + r" \{.*?\} " + name + r";", current, re.S).group()
            fields[name] = [field for field in fields[name] if re.search(r"\b" + field + r"\s*;", current_declaration)]
            result.append(declaration.replace(name, "Reference_" + name))
    result += ["static union { Reference_itBombHei_ItemVars bombhei; Reference_itDosei_ItemVars dosei; Reference_itPeachTurnip_ItemVars peachturnip; } reference_vars;",
               "static void reference_project(Item* output) { switch(output->kind) {"]
    for kind, member, saved in [("BombHei", "bombhei", "xDE8"), ("Dosei", "dosei", "xDE0"), ("Peach_Turnip", "peachturnip", "xDE0_scl")]:
        type_name = {"bombhei": "itBombHei_ItemVars", "dosei": "itDosei_ItemVars", "peachturnip": "itPeachTurnip_ItemVars"}[member]
        result.append("case It_Kind_" + kind + ":")
        for field in fields[type_name]:
            result.append(f"output->xDD4_itemVar.{member}.{field} = reference_vars.{member}.{field};")
        result.append(f"output->throw_scale = reference_vars.{member}.{saved}; break;")
    result.append("default: break; } }")
    return "\n".join(result) + "\n"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--iso", type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    original, candidate, legacy, receipts = [], [], [], {}
    for path, names in SOURCES.items():
        previous = subprocess.check_output(["git", "show", ORIGINAL_REF + ":" + path],
                                           cwd=args.root).decode("utf-8")
        current = (args.root / path).read_text(encoding="utf-8")
        receipts[path] = {"original_sha256": hashlib.sha256(previous.encode()).hexdigest(),
                          "candidate_sha256": hashlib.sha256(current.encode()).hexdigest()}
        for name in names:
            reference = body(previous, name)
            legacy.append(renamed(reference_private(reference), "legacy_"))
            if name == "it_802BD4AC":
                # These are representation repairs pinned by the retail stores,
                # not a hand-authored equivalent of the constructor or its order.
                bomb = "item->xDD4_itemVar.peachturnip.xDE8_scl = item->scl;"
                assert reference.count(bomb) == 1
                reference = reference.replace(bomb, "item->xDD4_itemVar.bombhei.xDE8 = item->scl;")
                saturn = "case It_Kind_Dosei:\n                item->xDD4_itemVar.peachturnip.xDE0_scl = item->scl;"
                assert reference.count(saturn) == 1
                reference = reference.replace(saturn, "case It_Kind_Dosei:\n                item->xDD4_itemVar.dosei.xDE0 = item->scl;")
            original.append(renamed(reference_private(reference), "console_"))
            candidate.append(renamed(body(current, name), "candidate_"))
    original.insert(0, reference_layout(args.root, receipts))
    # Shared absolute scale body is unchanged and runs for every variant.
    scale_path = "src/melee/it/it_2725.c"
    scale_source = (args.root / scale_path).read_text()
    receipts[scale_path] = {"candidate_sha256": hashlib.sha256(scale_source.encode()).hexdigest()}
    candidate.insert(0, body(scale_source, "it_80274484"))
    init_path = "src/melee/it/item.c"
    init_source = (args.root / init_path).read_text()
    initialization = re.search(r"    item_data->scl = item_data->xCC_item_attr->x60_scale;\n    item_data->throw_scale = 1\.0f;", init_source)
    if not initialization:
        raise ValueError("missing adjacent actual allocator scale defaults")
    candidate.insert(0, "// Exact allocator initialization slice (not full allocation fixture).\nstatic void candidate_initialize_scale(Item* item_data) {\n" + initialization.group() + "\n}\n")
    receipts[init_path] = {"candidate_sha256": hashlib.sha256(init_source.encode()).hexdigest(),
        "initialization_slice_sha256": hashlib.sha256(initialization.group().encode()).hexdigest()}
    for name, blocks in [("original", original), ("candidate", candidate), ("legacy", legacy)]:
        (args.output / ("peach_rare_scale_" + name + ".inc")).write_text(
            "\n\n".join(blocks) + "\n", encoding="utf-8")
    verified = False
    if args.iso:
        with args.iso.open("rb") as stream:
            stream.seek(0x420)
            dol = struct.unpack(">I", stream.read(4))[0]
            stream.seek(dol)
            header = stream.read(0x100)
            offsets = struct.unpack_from(">18I", header, 0)
            addresses = struct.unpack_from(">18I", header, 0x48)
            sizes = struct.unpack_from(">18I", header, 0x90)
            address, size = RETAIL["address"], RETAIL["size"]
            section = next(i for i in range(18) if addresses[i] <= address and
                           address + size <= addresses[i] + sizes[i])
            stream.seek(dol + offsets[section] + address - addresses[section])
            code = stream.read(size)
        assert hashlib.sha256(code).hexdigest() == RETAIL["sha256"]
        for address, instruction in STORES.items():
            assert struct.unpack_from(">I", code, address - RETAIL["address"])[0] == instruction
        verified = True
    (args.output / "peach_rare_scale_source.json").write_text(json.dumps({
        "original_ref": ORIGINAL_REF, "sources": receipts,
        "body_count_per_variant": sum(map(len, SOURCES.values())), "retail_ctor": RETAIL,
        "retail_store_instructions": {hex(k): hex(v) for k, v in STORES.items()},
        "retail_verified": verified,
        "reference_adjustment": "Only Bomb/Saturn constructor aliases match retail offsets; original private storage retained in test-only historical types, then named fields/saved scale projected into new Item representation",
        "scope": "Actual constructor/spawn/throw/drop/Bomb state10 source and common absolute scale body; no authored ordinary-item API or gameplay",
    }, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
