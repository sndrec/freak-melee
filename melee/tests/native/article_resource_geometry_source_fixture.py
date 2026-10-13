"""Extract unchanged stock helper bodies for the bounded resource geometry test."""
import re
import json
from pathlib import Path


def body(source, name):
    match = re.search(r"^[^\n;{}]*\b" + re.escape(name) + r"\s*\([^;{}]*\)\s*\{", source, re.M)
    if not match:
        raise ValueError(f"missing original definition: {name}")
    opening = source.index("{", match.start())
    depth = 1
    end = opening + 1
    while depth:
        if source[end] == "{":
            depth += 1
        elif source[end] == "}":
            depth -= 1
        end += 1
    return source[match.start():end]


def effect_payloads(sdk, output):
    source=body((sdk / "src/melee/ef/efasync.c").read_text(encoding="utf-8"), "efAsync_Dispatch")
    cases=list(re.finditer(r"^    case (0x[0-9A-Fa-f]+):",source,re.M))
    position=[];branches=[]
    for i,c in enumerate(cases):
        tail=source[c.end():cases[i+1].start() if i+1<len(cases) else len(source)]
        if "break;" not in tail: continue
        tail=tail[:tail.index("break;")+6]
        if re.search(r"efLib_CreateGenerator\([^,]+, va_arg\(vlist, Vec3\*\)\)",tail) and tail.count("va_arg")==1:
            position.append(c.group(1));branches.append(c.group(0)+tail)
    for first,last in [("0x3EF","0x3F1"),("0x3F1","0x3F3")]:
        begin=next(c.start() for c in cases if c.group(1)==first)
        end=next(c.start() for c in cases if c.group(1)==last)
        branches.append(source[begin:end])
    text="EF_SpawnResult stock_resource_dispatch(s32 gfx_id,HSD_GObj* gobj,va_list vlist) {\n"
    text+="EF_SpawnResult spawned={0}; Vec3* va_vec3; Vec3 translate; float f32_1; (void)gobj;\nswitch(gfx_id) {\n"
    text+="\n".join(branches)+"default: abort(); } return spawned; }\n"
    text+="static const int position_effects[] = {"+", ".join(position)+"};\n"
    (output.parent / "resource_effect_payloads.h").write_text(text,encoding="utf-8",newline="\n")
    (output.parent / "resource_effect_payloads.json").write_text(json.dumps(position),encoding="utf-8",newline="\n")


def extract(sdk, output):
    selected = {
        "melee/it/kinds/itlinkhookshot.c": ["it_802A3D90", "it_802A43B8", "it_802A43EC",
                                            "it_802A4420", "it_802A4454", "it_802A2EE4_inline"],
        "melee/it/kinds/itsamusgrapple.c": ["samus_grapple_setup_tail", "samus_grapple_setup_pos"],
        "melee/lb/lb_00B0.c": ["lb_8000B9D8", "lb_8000BA0C"],
        "sysdolphin/baselib/jobj.c": ["HSD_JObjWalkTree0", "HSD_JObjWalkTree"],
        "sysdolphin/baselib/aobj.c": ["HSD_AObjSetRate"],
        "melee/ef/efasync.c": [],
    }
    text = ["// Unchanged actual source bodies; source hashes recorded by runner."]
    for path, names in selected.items():
        source = (sdk / "src" / path).read_text(encoding="utf-8")
        text += [f"// {path}: {name}\n" + body(source, name) for name in names]
    output.write_text("\n\n".join(text) + "\n", encoding="utf-8", newline="\n")
    effect_payloads(sdk, output)
    return selected


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk", type=Path, required=True,
                        help="source root containing src/; frozen SDK or checkout")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    extract(args.sdk.resolve(), args.output.resolve())
