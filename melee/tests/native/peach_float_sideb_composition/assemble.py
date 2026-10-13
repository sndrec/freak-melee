"""Prepare a generic50-dependent candidate from immutable composition48."""
import argparse
import hashlib
import json
import re
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BASE = "tests/native/peach_full_fighter_composition/candidate.fms"


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--sdk", type=Path, default=ROOT / ".local/baseline48/sdk")
    a = p.parse_args()
    original = (ROOT / BASE).read_text()
    source = original
    declarations = []
    policies = []
    for name, entry in (("float", "func continue_float_input()"),
                        ("sideb", "func side_start_reset()")):
        component = (HERE / (name + ".fms")).read_text()
        declaration, policy = component.split(entry, 1)
        declarations.append(declaration)
        policies.append(entry + policy)
    source = source.replace("state int parasol_request_kind", "\n".join(declarations) +
                            "\nstate int parasol_request_kind", 1)
    source += "\n" + "\n".join(policies)
    source = source.replace(
        "# Float, Side-B and Parasol travel/animation remain inherited pending generic contracts.",
        "# Float/SideB require generic50 compilation; Parasol travel/common protocol remains inherited.",
    )
    assert "behavior independent" not in source
    fields = re.findall(r"(?m)^state (bool|int|float) (\w+)$", source)
    functions = re.findall(r"(?m)^(?:export )?func (\w+)\(", source)
    parameters = re.findall(r"(?m)^(?:parameter|attribute) (?:float|int|bool) (\w+)", source)
    phases = re.findall(r"(?m)^\s+on state_(\d+)_(animation|interrupt|physics|collision):", source)
    assert len(fields) == len({name for _, name in fields}) == 18
    assert len(functions) == len(set(functions))
    assert len(parameters) == len(set(parameters))
    assert len(phases) == len(set(phases)) == 90
    assert all(int(motion) != 356 for motion, _ in phases)
    (HERE / "candidate.fms").write_text(source)
    paths = [
        "src/melee/ft/kinds/ftPeach/ftpeachfloat.c",
        "src/melee/ft/kinds/ftPeach/ftpeachfloatfall.c",
        "src/melee/ft/kinds/ftPeach/ftpeachfloatattack.c",
        "src/melee/ft/kinds/ftPeach/ftpeachspecials.c",
        "src/melee/ft/kinds/ftPeach/types.h",
        "src/melee/ft/kinds/ftPeach/ftpeach.c",
        "src/melee/ft/ftmovement.c", "src/melee/ft/ftcommon.c", "src/melee/ft/ft_081B.c",
        "src/melee/ft/ft_0DF1.c", "src/melee/ft/fighter.c",
        "src/melee/ft/kinds/ftCommon/ftCo_Fall.c",
        "src/melee/ft/kinds/ftCommon/ftCo_Jump.c",
        "src/melee/ft/kinds/ftCommon/ftCo_JumpAerial.c",
        "src/melee/ft/kinds/ftCommon/ftCo_AttackAir.c",
        "src/melee/ft/kinds/ftCommon/ftCo_LandingAir.c",
    ]
    manifest = {
        "status": "authored draft; generic50 source/generated acceptance pending",
        "source_snapshot": a.sdk.parent.name,
        "accepted_inputs": {BASE: hashlib.sha256((ROOT / BASE).read_bytes()).hexdigest()},
        "original_source_hashes": {path: hashlib.sha256((a.sdk / path).read_bytes()).hexdigest()
                                   for path in paths},
        "fighter_state_fields": fields, "fighter_state_payload_bytes":
            sum(1 if typ == "bool" else 4 for typ, _ in fields),
        "function_count": len(functions), "function_names": functions,
        "fighter_parameter_declarations": len(parameters), "authored_motion_phases": len(phases),
        "new_motion_phases": 56, "null_motion": 356,
        "remaining_inherited_motions": [361, 362, 363, 364, 369, 370],
    }
    (HERE / "components.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print("Draft:", len(phases), "authored phases;", len(fields), "fields;",
          len(functions), "helpers; generic50 acceptance pending")


if __name__ == "__main__":
    main()
