"""Pin remaining Peach ownership decisions against a supplied frozen source SDK."""
import argparse
import hashlib
import json
import re
from pathlib import Path

HERE = Path(__file__).resolve().parent


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--sdk", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    a = p.parse_args()
    sdk = a.sdk.resolve()
    sources = {}
    paths = list((sdk / "src/melee/ft/kinds/ftPeach").glob("*.c"))
    paths += list((sdk / "src/melee/it/kinds").glob("itpeach*.c"))
    paths += [sdk / path for path in (
        "src/melee/ft/ftdata.c", "src/melee/ft/fighter.c", "src/melee/ft/ftcommon.c",
        "src/melee/ft/ftcpuattack.c", "src/melee/ft/ftdynamics.c",
        "src/melee/ft/ft_08A1.c", "src/melee/ft/ft_0D27.c", "src/melee/ft/ft_0DF1.c",
        "src/melee/ft/native_fighter_runtime.h", "src/melee/ft/native_fighter_module.h",
        "src/melee/ft/native_fighter_module.c", "src/melee/ft/native_fighter_article_bindings.c",
        "src/melee/ft/kinds/ftCommon/ftCo_Fall.c",
        "src/melee/ft/kinds/ftCommon/ftCo_Jump.c",
        "src/melee/ft/kinds/ftCommon/ftCo_JumpAerial.c",
        "src/melee/ft/kinds/ftCommon/ftCo_AttackAir.c",
        "src/melee/ft/kinds/ftCommon/ftCo_Landing.c",
        "src/melee/ft/kinds/ftCommon/ftCo_LandingAir.c",
        "src/melee/ft/kinds/ftCommon/ftCo_Throw.c",
        "src/melee/ft/kinds/ftCommon/ftCo_AppealS.c",
        "src/melee/ft/kinds/ftCommon/ftCo_ItemParasolOpen.c",
        "src/melee/ft/kinds/ftCommon/ftCo_ItemParasolFall.c",
        "src/melee/ft/kinds/ftCommon/ftCo_ItemParasolFallSpecial.c",
        "src/melee/ft/kinds/ftCommon/inlines.h",
        "src/melee/ft/kinds/ftPeach/types.h",
    )]
    references = []
    for path in sorted(set(paths)):
        text = path.read_text()
        relative = path.relative_to(sdk).as_posix()
        sources[relative] = hashlib.sha256(path.read_bytes()).hexdigest()
        for line, source in enumerate(text.splitlines(), 1):
            if re.search(r"Ft_Kind_Peach|ftPe_|it_802BDA40|it_802BDD40|it_802BDDB4", source):
                references.append({"path": relative, "line": line, "source": source.strip()})
    path = sdk / "src/melee/ft/kinds/ftPeach/ftpeach.c"
    text = path.read_text()
    table = text[text.index("MotionState ftPe_Init_MotionStateTable"):]
    table = table[:table.index("};")]
    rows = re.findall(r"\{\s*// ([^\n]+)\n(.*?)\n    \},", table, re.S)
    candidate = (HERE / "candidate.fms").read_text()
    phase_names = {"animation": "anim", "interrupt": "iasa", "physics": "phys", "collision": "coll"}
    authored = {(int(n), phase_names[phase]) for n, phase in re.findall(
        r"(?m)^\s+on state_(\d+)_(animation|interrupt|physics|collision):", candidate)}
    slots = []
    for comment, row in rows:
        motion = int(comment.split("=")[-1])
        values = [x.strip() for x in row.strip().split(",") if x.strip()]
        # The retail aggregate has three metadata words then four phases/camera.
        assert len(values) == 8, (motion, values)
        phases = dict(zip(("anim", "iasa", "phys", "coll"), values[3:7]))
        slots.append({"motion": motion, "name": comment.split("=")[0].strip(),
                      "phases": {phase: {"original": callback,
                                          "authored": (motion, phase) in authored}
                                 for phase, callback in phases.items()},
                      "camera": values[7]})
    assert len(slots) == 30
    nonnull = sum(x["original"] != "NULL" for row in slots for x in row["phases"].values())
    assert nonnull == 114
    fields = re.findall(r"(?m)^state (bool|int|float) (\w+)$", candidate)
    article_states = {kind: int(n) for kind, n in re.findall(
        r"(?m)^article (\w+) states (\d+):", candidate)}
    result = {
        "snapshot": sdk.parent.name, "status": "closure census, inherited candidate only",
        "nonnull_motion_phases": nonnull, "authored_motion_phases": len(authored),
        "motion_rows": slots, "article_states": article_states,
        "fighter_state_fields": fields,
        "fighter_payload_bytes": sum(1 if t == "bool" else 4 for t, _ in fields),
        "source_hashes": sources, "references": references,
        "accepted_components": json.loads((HERE / "components.json").read_text()),
    }
    a.output.write_text(json.dumps(result, indent=2) + "\n")
    print("Peach census:", len(slots), "rows,", nonnull, "nonnull phases,",
          len(authored), "authored phases;", len(article_states), "article declarations")


if __name__ == "__main__":
    main()
