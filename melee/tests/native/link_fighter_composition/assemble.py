"""Compose accepted behavior definitions directly; never mutate their inputs."""
from pathlib import Path
import re
ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
NATIVE = HERE.parent

def blocks(path):
    source = path.read_text(encoding="utf-8")
    starts = list(re.finditer(r"^(?:article|func|export func|action|move|state|attribute|parameter)\s", source, re.M))
    return [(source[m.start():starts[i+1].start() if i+1 < len(starts) else len(source)], path) for i,m in enumerate(starts)]

def compose(family):
    sources = [ROOT / "examples/link_clink_wip.fms", NATIVE / "link_article_cleanup/bow.component.fms", NATIVE / "link_article_cleanup/boomerang.component.fms", NATIVE / "link_grounded_composition/tether.component.fms", NATIVE / "link_grounded_composition/ground.component.fms", NATIVE / "link_article_cleanup/cleanup.component.fms", HERE / "lifecycle.component.fms", HERE / "trail.component.fms", HERE / "dair.component.fms", HERE / "shield.component.fms", HERE / "sword_combo.component.fms"]
    if family == "young_link": sources += [NATIVE / "link_article_cleanup/milk.component.fms", HERE / "young_taunt.component.fms"]
    selected = []
    for source in sources:
        for block,path in blocks(source):
            first=block.splitlines()[0]
            if first.startswith("action CleanupHarness:"): continue
            if first.startswith("article It_Kind_CLink_Bomb"): continue
            selected.append((block,path))
    phase_inputs = [(NATIVE / "link_bow_arrow/link/behavior.fms", {"move Bow:","action BowPhases:"}), (NATIVE / "link_boomerang_fixture.fms", {"move Boomerang:","action BoomerangPhases:"}), (NATIVE / "link_aircatch/link.fixture.fms", {"action TetherInput:","action TetherAirPhases:"})]
    for path,names in phase_inputs:
        for block,path in blocks(path):
            if block.splitlines()[0] in names:
                if block.startswith("action BowPhases:"):
                    block=re.sub(r"    on damage:[\s\S]*", "", block)
                selected.append((block,path))
    groups=[[],[],[],[],[]]; declarations={}; inputs=set()
    for block,path in selected:
        first=block.splitlines()[0];inputs.add(path)
        group=0 if first.startswith(("state ","attribute ","parameter ")) else 1 if first.startswith(("func ","export func ")) else 2 if first.startswith("article ") else 3 if first.startswith("move ") else 4
        if group==0:
            key=tuple(first.split()[:3])
            if key in declarations:
                if first!=declarations[key]:raise ValueError("conflicting declaration "+first)
                continue
            declarations[key]=first
        groups[group].append(block)
    source="# UNREGISTERED composed fighter candidate; common/rig closure remains explicit.\n\n"+"\n".join(b for group in groups for b in group)
    source=re.sub(r"(state\.boomerang_vertical_offset = attrs\.boomerang_vertical_offset)(?=\s*$)", r"\1 * model.instance_scale_y", source, flags=re.M)
    if family=="link":
        source=source.replace('    if state.milk_handle != -1:\n        if articles.ref(state.milk_handle, It_Kind_CLink_Milk).exists:\n            articles.signal(state.milk_handle, It_Kind_CLink_Milk, "remove", 0)\n        state.milk_handle = -1\n', "")
        source=source.replace("        state.milk_handle = -1\n", "")
    else:
        source=source.replace("It_Kind_Link_", "It_Kind_CLink_").replace("carried_shield_recoil_sound = 160106", "carried_shield_recoil_sound = 70106")
        source=source.replace("tether_extend_sound = 160073", "tether_extend_sound = 70073").replace("tether_retract_sound = 160076", "tether_retract_sound = 70076")
        source=source.replace("    on load:\n", "    on load:\n        fighter.enable_walljump()\n",1)
    return source,sorted(inputs)
