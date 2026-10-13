"""Compose accepted Peach components, never modifying their source namespaces."""
import hashlib
import json
import re
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
INPUTS = {
    "pluck_smash": "tests/native/peach_pluck_smash/candidate.fms",
    "counter": "tests/native/peach_counter_full_phase/counter.fms",
}


def main():
    sources = {name: (ROOT / path).read_text() for name, path in INPUTS.items()}
    source = sources["pluck_smash"]
    assert source.count("article It_Kind_Peach_Toad states 2:") == 1
    source = re.sub(
        r"article It_Kind_Peach_Toad states 2:.*?(?=article It_Kind_Peach_Turnip)",
        "", source, flags=re.S,
    )
    declarations, policy = sources["counter"].split("func pause_toad():", 1)
    source = source.replace("state int parasol_request_kind", declarations +
                            "\nstate int parasol_request_kind", 1)
    source += "\nfunc pause_toad():" + policy
    source = source.replace(
        "# Float, Side-B, Toad and Parasol travel/animation remain pending.",
        "# Float, Side-B and Parasol travel/animation remain inherited pending generic contracts.",
    ).replace(
        "# Partial Parasol/Toad tables below exist solely for accepted resource callbacks.",
        "# Partial Parasol table exists solely for accepted resource callbacks.",
    )
    assert "behavior independent" not in source
    fields = re.findall(r"(?m)^state (bool|int|float) (\w+)$", source)
    assert len(fields) == len({name for _, name in fields}) == 15
    (HERE / "candidate.fms").write_text(source)
    paths = list(INPUTS.values()) + [
        "tests/native/peach_pluck_smash/test.c",
        "tests/native/peach_pluck_smash/fixture.py",
        "tests/native/peach_counter_full_phase/test.c",
        "tests/native/peach_counter_full_phase/fixture.py",
        "tests/native/peach_fighter_composition/candidate.fms",
    ]
    manifest = {
        "source_inputs": {path: hashlib.sha256((ROOT / path).read_bytes()).hexdigest()
                          for path in paths},
        "changes": [
            "Accepted pluck/smash/setup/Turnip/cleanup/Parasol producer retained verbatim.",
            "Partial Toad table replaced with complete counter/Toad/Spore component.",
            "34 custom motion phases; all five article declarations coexist.",
            "Float, Side-B, full Parasol flight/common routing remain inherited, not activated.",
        ],
    }
    (HERE / "components.json").write_text(json.dumps(manifest, indent=2) + "\n")


if __name__ == "__main__":
    main()
